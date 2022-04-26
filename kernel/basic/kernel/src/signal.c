#include <kernel/signal.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>

int32_t  proc_signal_setup(uint32_t entry) {
	proc_t* cproc = get_current_proc();
	cproc->space->signal.entry = entry;

	cproc->space->signal.stack = proc_stack_alloc(cproc);
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

