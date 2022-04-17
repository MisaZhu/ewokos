#include <kernel/signal.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/schedule.h>

int32_t  proc_signal_setup(uint32_t entry) {
	proc_t* cproc = get_current_proc();
	cproc->space->signal.entry = entry;
	return 0;	
}

void  proc_signal_send(context_t* ctx, proc_t* proc, int32_t sig_no) {
	proc_t* cproc = get_current_proc();
	ctx->gpr[0] = -1;
	if(proc == NULL || proc->space->signal.entry == 0) 
		return;
	ctx->gpr[0] = 0;
	
	proc->space->signal.sig_no = sig_no;
	proc->space->signal.saved_state = proc->info.state;
	proc->space->signal.saved_block_by = proc->info.block_by;
	proc->space->signal.saved_block_event = proc->block_event;
	proc->space->signal.start = true;

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

	cproc->info.state = cproc->space->signal.saved_state;
	cproc->info.block_by = cproc->space->signal.saved_block_by;
	cproc->block_event = cproc->space->signal.saved_block_event;
	memcpy(ctx, &cproc->space->signal.ctx, sizeof(context_t));
	schedule(ctx);
}

