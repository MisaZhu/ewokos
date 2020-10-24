#include <kernel/signal.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/schedule.h>

int32_t  proc_signal_setup(uint32_t entry) {
	_current_proc->space->signal.state = SIG_STATE_IDLE;
	_current_proc->space->signal.entry = entry;
	return 0;	
}

void  proc_signal_send(context_t* ctx, proc_t* proc, int32_t sig_no) {
	ctx->gpr[0] = -1;
	if(proc == NULL || proc->space->signal.entry == 0 || proc->space->signal.state != SIG_STATE_IDLE)
		return;
	ctx->gpr[0] = 0;
	
	proc->space->signal.proc_state = proc->info.state;
	memcpy(&proc->space->signal.ctx, &proc->ctx, sizeof(context_t));

	proc->ctx.pc = proc->ctx.lr = proc->space->signal.entry;
	proc->ctx.gpr[0] = sig_no;
	proc->info.state = RUNNING;
	if(proc != _current_proc)
		proc_switch(ctx, proc, true);
	else 
		memcpy(ctx, &proc->space->signal.ctx, sizeof(context_t));
}

void proc_signal_end(context_t* ctx) {
	_current_proc->space->signal.state = SIG_STATE_IDLE;
	if(_current_proc->space->signal.entry == 0)
		return;

	proc_ready(_current_proc);
	memcpy(ctx, &_current_proc->space->signal.ctx, sizeof(context_t));
	schedule(ctx);
}

