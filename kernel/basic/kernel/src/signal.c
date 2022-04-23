#include <kernel/signal.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <dev/ipi.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>

int32_t  proc_signal_setup(uint32_t entry) {
	proc_t* cproc = get_current_proc();
	cproc->space->signal.entry = entry;

	/*uint32_t page = kalloc4k();
	map_page(cproc->space->vm,
			page,
			V2P(page),
			AP_RW_RW, 0);
	cproc->space->signal.stack = page;
	*/
	cproc->space->signal.stack = (uint32_t)proc_malloc(cproc, THREAD_STACK_PAGES*PAGE_SIZE);
	return 0;	
}

void  proc_signal_send(context_t* ctx, proc_t* proc, int32_t sig_no) {
	proc_t* cproc = get_current_proc();
	ctx->gpr[0] = -1;
	if(proc == NULL || proc->space->signal.entry == 0) 
		return;
	ctx->gpr[0] = 0;
	
	proc_save_state(proc, &proc->space->signal.saved_state);
	proc->space->signal.sig_no = sig_no;
	proc->space->signal.do_switch = true;

	if(proc->info.core == cproc->info.core) {
		proc->info.state = RUNNING;
		proc_switch(ctx, proc, true);
	}
	else {
		proc_ready(proc);
#ifdef KERNEL_SMP
		ipi_send(proc->info.core);
#endif
	}
}

void proc_signal_end(context_t* ctx) {
	proc_t* cproc = get_current_proc();
	if(cproc->space->signal.entry == 0)
		return;

	proc_restore_state(ctx, cproc, &cproc->space->signal.saved_state);
	schedule(ctx);
}

