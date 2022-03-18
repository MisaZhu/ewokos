#include <dev/timer.h>
#include <dev/ipi.h>
#include <kernel/irq.h>
#include <kernel/interrupt.h>
#include <kernel/system.h>
#include <kernel/schedule.h>
#include <kernel/kernel.h>
#include <kernel/proc.h>
#include <kernel/kevqueue.h>
#include <kernel/interrupt.h>
#include <string.h>
#include <signals.h>
#include <kernel/signal.h>
#include <kernel/hw_info.h>
#include <kprintf.h>
#include <mm/kalloc.h>

uint32_t _kernel_sec = 0;
uint64_t _kernel_usec = 0;
static uint64_t _schedule_tic = 0;
static uint64_t _timer_tic = 0;

#ifdef KERNEL_SMP
void ipi_enable_all(void) {
	uint32_t i;
	for(i=0; i<get_cpu_cores(); i++) {
		ipi_enable(i);
	}
}

static uint32_t _intr_core = 0;
void ipi_send_core(void) {
	if(_intr_core >= get_cpu_cores())
		_intr_core = 0;
	if(proc_num_in_core(_intr_core) > 0)
		ipi_send(_intr_core);
	_intr_core++;
}
#endif

static inline void irq_do_uart0(context_t* ctx) {
	(void)ctx;
	interrupt_send(ctx, SYS_INT_UART0);
}

static inline void irq_do_timer0(context_t* ctx) {
	uint64_t usec = timer_read_sys_usec();
	if(_kernel_usec == 0) {
		_kernel_usec = usec;
	}
	else {
		uint64_t usec_gap = usec - _kernel_usec;
		_kernel_usec = usec;
		_schedule_tic += usec_gap;
		_timer_tic += usec_gap;
		if(_timer_tic >= 1000000) { //1 sec
			_kernel_sec++;
			_timer_tic = 0;
		}

		if(_schedule_tic >= 20000) { //20 msec, 50 times scheduling per second
			_schedule_tic = 0;
		}
		renew_kernel_tic(usec_gap);
	}
	timer_clear_interrupt(0);
	if(_schedule_tic == 0) {
#ifdef KERNEL_SMP
		ipi_send_core();
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
		if(proc_num_in_core(cid) > 0)
			schedule(ctx);
#endif
	}
}

inline void irq_handler(context_t* ctx) {
	__irq_disable();

#ifdef KERNEL_SMP
	if(kernel_lock_check() > 0)
		return;

	uint32_t cid = get_core_id();
	kernel_lock();
	_irq_handler(cid, ctx);
	kernel_unlock();
#else
	_irq_handler(0, ctx);
#endif
}

static void dump_ctx(context_t* ctx) {
	printf("ctx dump:\n"
		"  cpsr=0x%x\n"
		"  pc=0x%x\n"
		"  sp=0x%x\n"
		"  lr=0x%x\n",
		ctx->cpsr,
		ctx->pc,
		ctx->sp,
		ctx->lr);
	uint32_t i;
	for(i=0; i<13; i++) 
		printf("  r%d: 0x%x\n", i, ctx->gpr[i]);
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
			AP_RW_RW);
	flush_tlb();
	return 0;
}

void undef_abort_handler(context_t* ctx, uint32_t status) {
	(void)ctx;
	uint32_t core = get_core_id();
#ifdef KERNEL_SMP
	kernel_lock();
#endif
	proc_t* cproc = get_current_proc();
	if(cproc == NULL) {
		printf("_kernel, undef instrunction abort!! (core %d)\n", core);
		dump_ctx(ctx);
		while(1);
	}

	if(ctx->pc >= cproc->space->heap_size || //in proc heap only
			copy_on_write(cproc, ctx->pc) != 0) {
		printf("pid: %d(%s), undef instrunction abort!! (core %d)\n", cproc->info.pid, cproc->info.cmd, core);
		dump_ctx(&cproc->ctx);
		proc_signal_send(ctx, cproc, SYS_SIG_STOP);
	}
#ifdef KERNEL_SMP
	kernel_unlock();
#endif
}

void prefetch_abort_handler(context_t* ctx, uint32_t status) {
	(void)ctx;
	uint32_t core = get_core_id();
#ifdef KERNEL_SMP
	kernel_lock();
#endif
	proc_t* cproc = get_current_proc();
	if(cproc == NULL) {
		printf("_kernel, prefetch abort!! (core %d)\n", core);
		dump_ctx(ctx);
		while(1);
	}

	if((status & 0xD) != 0xD || //permissions fault only
			ctx->pc >= cproc->space->heap_size || //in proc heap only
			copy_on_write(cproc, ctx->pc) != 0) {
		printf("pid: %d(%s), prefetch abort!! (core %d)\n", cproc->info.pid, cproc->info.cmd, core);
		dump_ctx(&cproc->ctx);
		proc_signal_send(ctx, cproc, SYS_SIG_STOP);
	}
#ifdef KERNEL_SMP
	kernel_unlock();
#endif
}

void data_abort_handler(context_t* ctx, uint32_t addr_fault, uint32_t status) {
	(void)ctx;
#ifdef KERNEL_SMP
	kernel_lock();
#endif
	proc_t* cproc = get_current_proc();
	if(cproc == NULL) {
		printf("_kernel, data abort!! at: 0x%X\n", addr_fault);
		dump_ctx(ctx);
		while(1);
	}

	if((status & 0xD) != 0xD || //permissions fault only
			addr_fault >= cproc->space->heap_size || //in proc heap only
			copy_on_write(cproc, addr_fault) != 0) {
		printf("pid: %d(%s), core: %d, data abort!! at: 0x%X, code: 0x%X\n", cproc->info.pid, cproc->info.cmd, cproc->info.core, addr_fault, status);
		dump_ctx(&cproc->ctx);
		proc_signal_send(ctx, cproc, SYS_SIG_STOP);
	}
#ifdef KERNEL_SMP
	kernel_unlock();
#endif
}

void irq_init(void) {
	irq_arch_init();
	interrupt_init();
	_kernel_sec = 0;
	_kernel_usec = 0;
	_schedule_tic = 0;
	_timer_tic = 0;
	//gic_set_irqs( IRQ_UART0 | IRQ_TIMER0 | IRQ_KEY | IRQ_MOUSE | IRQ_SDC);
	irq_enable(IRQ_TIMER0 | IRQ_UART0);

#ifdef KERNEL_SMP
	_intr_core = 0;
	ipi_enable_all();
#endif
}
