#include <kernel/signal.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/schedule.h>

int32_t  proc_signal_setup(uint32_t entry) {
	proc_t* cproc = get_current_proc();
	cproc->space->signal.state = SIG_STATE_IDLE;
	cproc->space->signal.entry = entry;
	return 0;	
}

void  proc_signal_send(context_t* ctx, proc_t* proc, int32_t sig_no) {
	proc_t* cproc = get_current_proc();
	ctx->gpr[0] = -1;
	if(proc == NULL || proc->space->signal.entry == 0 || proc->space->signal.state != SIG_STATE_IDLE)
		return;
	ctx->gpr[0] = 0;
	
	proc->space->signal.saved_state = proc->info.state;
	proc->space->signal.saved_block_by = proc->info.block_by;
	proc->space->signal.saved_block_event = proc->block_event;
	memcpy(&proc->space->signal.ctx, &proc->ctx, sizeof(context_t));

	proc->ctx.pc = proc->ctx.lr = proc->space->signal.entry;
	proc->ctx.gpr[0] = sig_no;
	if(proc != cproc) {
		proc->info.state = RUNNING;
		proc_switch(ctx, proc, true);
	}
	else 
		memcpy(ctx, &proc->space->signal.ctx, sizeof(context_t));
}

void proc_signal_end(context_t* ctx) {
	proc_t* cproc = get_current_proc();
	cproc->space->signal.state = SIG_STATE_IDLE;
	if(cproc->space->signal.entry == 0)
		return;

	cproc->info.state = cproc->space->signal.saved_state;
	cproc->info.block_by = cproc->space->signal.saved_block_by;
	cproc->block_event = cproc->space->signal.saved_block_event;
	memcpy(ctx, &cproc->space->signal.ctx, sizeof(context_t));
	schedule(ctx);
}

