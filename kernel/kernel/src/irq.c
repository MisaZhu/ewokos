#include <dev/timer.h>
#include <kernel/irq.h>
#include <kernel/interrupt.h>
#include <kernel/system.h>
#include <kernel/schedule.h>
#include <kernel/kernel.h>
#include <kernel/proc.h>
#include <kernel/kevqueue.h>
#include <kernel/interrupt.h>
#include <kernel/core.h>
#include <kstring.h>
#include <signals.h>
#include <kernel/signal.h>
#include <kernel/hw_info.h>
#include <kernel/svc.h>
#include <kprintf.h>
#include <mm/kalloc.h>
#include <stddef.h>

uint32_t _kernel_sec = 0;
uint64_t _kernel_usec = 0;

static uint64_t _last_usec = 0;
static uint32_t _schedule = 0;
static uint32_t _schedule_usec = 0;
static uint32_t _timer_tic = 0;
static uint32_t _sec_tic = 0;

#ifdef KERNEL_SMP

void ipi_enable_all(void) {
	uint32_t i;
	for(i=0; i<_sys_info.cores; i++) {
		ipi_enable(i);
	}
}

static inline void ipi_send_all(void) {
	uint32_t i;
	uint32_t cores = _sys_info.cores;
	for(i=0; i< cores; i++) {
		if(proc_have_ready_task(i))
			ipi_send(i);
	}
}

#endif

static inline void irq_do_uart0(context_t* ctx) {
	interrupt_send(ctx, SYS_INT_UART0);
}

static inline int32_t irq_do_timer0_interrupt(context_t* ctx) {
	return interrupt_send(ctx, SYS_INT_TIMER0);
}

static inline void irq_do_timer0(context_t* ctx) {
	(void)ctx;
	uint64_t usec = timer_read_sys_usec();
	int32_t do_schedule = 0;

	uint32_t usec_gap = usec - _last_usec;
	_last_usec = usec;
	_kernel_usec += usec_gap;
	_sec_tic += usec_gap;
	_timer_tic += usec_gap;
	if(_sec_tic >= 1000000) { //SEC_TIC sec
		_kernel_sec++;
		_sec_tic = 0;
		renew_kernel_sec();
	}
	if(renew_kernel_tic(usec_gap) == 0)
		do_schedule = 1;
	
	timer_clear_interrupt(0);

	if(_timer_tic >= _schedule_usec) {
		_timer_tic = 0;
		if(_schedule == 0) { //this tic not for schedule, do timer interrupt.
			_schedule = 1; //next tic for schedule
			if(irq_do_timer0_interrupt(ctx) == 0) //if timer set, don't do schedule
				return;
		}
		do_schedule = 1;
	}
	
	if(do_schedule) {
		_schedule = 0;
#ifdef KERNEL_SMP
		ipi_send_all();
#else
		schedule(ctx);
#endif
	}
}

static inline void _irq_handler(uint32_t cid, context_t* ctx) {
	uint32_t irqs = irq_gets();

	//handle irq
	if((irqs & IRQ_UART0) != 0) {
		irq_do_uart0(ctx);
	}
	else if(cid == 0 && (irqs & IRQ_TIMER0) != 0) {
		irq_do_timer0(ctx);
	}
	else {
#ifdef KERNEL_SMP
		ipi_clear(cid);
		if(proc_have_ready_task(cid))
			schedule(ctx);
#endif
	}
}

inline void irq_handler(context_t* ctx) {
	__irq_disable();
	if(kernel_lock_check() > 0)
		return;

	uint32_t cid = get_core_id();
	kernel_lock();
	_irq_handler(cid, ctx);
	kernel_unlock();
}



static int32_t copy_on_write(proc_t* proc, uint32_t v_addr) {
	v_addr = ALIGN_DOWN(v_addr, PAGE_SIZE);
	uint32_t phy_addr = resolve_phy_address(proc->space->vm, v_addr);
	char *page = kalloc4k();
	if(page == NULL) {
		return -1;
	}
	memcpy(page, (char*)P2V(phy_addr), PAGE_SIZE);
	unmap_page_ref(proc->space->vm, v_addr);
	map_page_ref(proc->space->vm,
			v_addr,
			V2P(page),
			AP_RW_RW, PTE_ATTR_WRBACK);
	flush_tlb();
	return 0;
}

void undef_abort_handler(context_t* ctx, uint32_t status) {
	(void)ctx;
	(void)status;
	__irq_disable();
	uint32_t core = get_core_id();
	proc_t* cproc = get_current_proc();
	if(cproc == NULL) {
		printf("_kernel, undef instrunction abort!! (core %d)\n", core);
		dump_ctx(ctx);
		halt();
	}

	printf("pid: %d(%s), undef instrunction abort!! (core %d)\n", cproc->info.pid, cproc->info.cmd, core);
	dump_ctx(&cproc->ctx);
	proc_exit(ctx, cproc, -1);
	//proc_signal_send(ctx, cproc, SYS_SIG_STOP);
}

void prefetch_abort_handler(context_t* ctx, uint32_t status) {
	(void)ctx;
	__irq_disable();
	uint32_t core = get_core_id();

	proc_t* cproc = get_current_proc();
	if(cproc == NULL) {
		printf("_kernel, prefetch abort!! (core %d)\n", core);
		dump_ctx(ctx);
		halt();
	}
	
	if(((status & 0x1D) == 0xD || //permissions fault only
		(status & 0x1F) == 0x6) && 
			ctx->pc < cproc->space->heap_size) { //in proc heap only
		if (kernel_lock_check() > 0)
			return;

		kernel_lock();
		int32_t res = copy_on_write(cproc, ctx->pc);
		kernel_unlock();
		if(res == 0)
			return;
	}

	printf("pid: %d(%s), prefetch abort!! (core %d) code:0x%x\n", cproc->info.pid, cproc->info.cmd, core, status);
	dump_ctx(&cproc->ctx);
	proc_exit(ctx, cproc, -1);
}

void data_abort_handler(context_t* ctx, uint32_t addr_fault, uint32_t status) {
	(void)ctx;
	__irq_disable();
	proc_t* cproc = get_current_proc();
	if(cproc == NULL) {
		printf("_kernel, data abort!! core: %d, at: 0x%X status: 0x%X\n", 
			get_core_id(), addr_fault, status);
		dump_ctx(ctx);
		halt();
	}

	if(((status & 0x1D) == 0xD || //permissions fault only
		(status & 0x1F) == 0x6) && 
			addr_fault < cproc->space->heap_size) { //in proc heap only
		if (kernel_lock_check() > 0)
			return;

		kernel_lock();
		int32_t res = copy_on_write(cproc, addr_fault);
		kernel_unlock();
		if(res == 0) 
			return;
	}

	printf("\npid: %d(%s), core: %d, data abort!! at: 0x%X, code: 0x%X\n", cproc->info.pid, cproc->info.cmd, cproc->info.core, addr_fault, status);
	dump_ctx(&cproc->ctx);
	proc_exit(ctx, cproc, -1);
}

void irq_init(void) {
	irq_arch_init();
	interrupt_init();
	_kernel_sec = 0;
	_kernel_usec = 0;
	_sec_tic = 0;
	_schedule = 0;
	_timer_tic = 0;
	_last_usec = timer_read_sys_usec();
	_schedule_usec = (1000000 / _kernel_config.schedule_freq) / 2;
	//gic_set_irqs( IRQ_UART0 | IRQ_TIMER0 | IRQ_KEY | IRQ_MOUSE | IRQ_SDC);
	irq_enable(IRQ_TIMER0 | IRQ_UART0);

#ifdef KERNEL_SMP
	ipi_enable_all();
#endif
}
