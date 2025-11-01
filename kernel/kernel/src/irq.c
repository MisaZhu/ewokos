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

static inline void irq_do_raw(context_t* ctx, uint32_t irq) {
	//kprintf("irq_raw: 0x%x\n", irq);
	irq_clear(irq);
	interrupt_send(ctx, irq);
}

static inline void irq_do_timer0(context_t* ctx) {
	(void)ctx;
	uint64_t usec = timer_read_sys_usec();
	uint32_t usec_gap = usec - _last_usec;

	_last_usec = usec;
	_kernel_usec += usec_gap;
	_sec_tic += usec_gap;

	if(_sec_tic >= 1000000) { //SEC_TIC sec
		_kernel_sec++;
		_sec_tic = 0;
		renew_kernel_sec();
	}
	renew_kernel_tic(usec_gap);
	
	timer_clear_interrupt(0);

#ifdef KERNEL_SMP
	ipi_send_all();
#else
	schedule(ctx);
#endif
}

static inline void _irq_handler(uint32_t cid, context_t* ctx) {
	uint64_t raw_irqs;
	uint32_t irq = irq_get();

	//handle irq
	if(irq > 0 && irq < IRQ_RAW_TOP) {
		irq_do_raw(ctx, irq);
	}
	else if(cid == 0 && irq == IRQ_TIMER0) {
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

static int32_t copy_on_write(proc_t* proc, ewokos_addr_t v_addr) {
	v_addr = ALIGN_DOWN(v_addr, PAGE_SIZE);
	ewokos_addr_t phy_addr = resolve_phy_address(proc->space->vm, v_addr);
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

	proc_exit(ctx, proc_get_proc(cproc), -1);
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
	/*kprintf("handle prefetch abort: %d, status: 0x%x, addr: 0x%x\n", cproc->info.pid, status, ctx->pc);

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
	*/

	printf("pid: %d(%s), prefetch abort!! (core %d) code:0x%x\n", cproc->info.pid, cproc->info.cmd, core, status);
	dump_ctx(&cproc->ctx);

	proc_exit(ctx, proc_get_proc(cproc), -1);
}

void data_abort_handler(context_t* ctx, ewokos_addr_t addr_fault, uint32_t status) {
	(void)ctx;
	__irq_disable();
	proc_t* cproc = get_current_proc();
	if(cproc == NULL) {
		printf("_kernel, data abort!! core: %d, at: 0x%X status: 0x%X\n", 
			get_core_id(), addr_fault, status);
		dump_ctx(ctx);
		halt();
	}
	//kprintf("handle data abort: %d, 0x%x, 0x%x\n", cproc->info.pid, status, addr_fault);

	uint32_t err = 0;
	const char* errmsg = "";
	uint32_t legel_addr_base = cproc->space->rw_heap_base;

	if((status & 0x5) == 0x5 || (status & 0xD) == 0xD) { //permissions fault only
		if(addr_fault >= legel_addr_base && addr_fault < cproc->space->heap_size) { //in proc heap only
			if (kernel_lock_check() > 0)
				return;

			kernel_lock();
			int32_t res = copy_on_write(cproc, addr_fault);
			kernel_unlock();
			if(res == 0) 
				return;
			err = 1;
			errmsg = "copy on write failed";
		}
		else {
			err = 2;
			errmsg = "illegel address";
		}
	}
	else {
		err = 3;
		errmsg = "access denied";
	}

	printf("\npid: %d(%s), core: %d, data abort at: 0x%X, status: 0x%X\n", 
			cproc->info.pid, cproc->info.cmd, cproc->info.core, addr_fault, status);
	if(err == 2) //illegel address
		printf("\terror: %s! heap(0x%X->0x%X)\n", errmsg, legel_addr_base, cproc->space->heap_size);
	else
		printf("\terror: %s!\n", errmsg);

	dump_ctx(ctx);
	proc_exit(ctx, proc_get_proc(cproc), -1);
}

void irq_init(void) {
	irq_arch_init();
	interrupt_init();
	_kernel_sec = 0;
	_kernel_usec = 0;
	_sec_tic = 0;
	_last_usec = timer_read_sys_usec();
	irq_enable(IRQ_TIMER0);

#ifdef KERNEL_SMP
	ipi_enable_all();
#endif
}
