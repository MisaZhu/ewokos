#include <kernel/signal.h>
#include <kernel/proc.h>
#include <stddef.h>
#include <kstring.h>

int32_t  proc_signal_setup(uint32_t entry) {
	_current_proc->space->signal.entry = entry;
	return 0;	
}

void  proc_signal_send(context_t* ctx, int32_t pid_to, int32_t sig_no) {
	ctx->gpr[0] = -1;
	proc_t* proc = proc_get(pid_to);
	if(proc == NULL || proc->space->signal.entry == 0 || proc->space->signal.state != SIG_STATE_IDLE)
		return;
	ctx->gpr[0] = 0;
	
	proc->space->signal.proc_state = proc->info.state;
	memcpy(&proc->space->ipc.ctx, &proc->ctx, sizeof(context_t));

	proc->ctx.pc = proc->ctx.lr = proc->space->signal.entry;
	proc->ctx.gpr[0] = sig_no;
	proc->info.state = RUNNING;
	proc_switch(ctx, proc, true);
}
