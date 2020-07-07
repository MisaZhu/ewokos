#include <kernel/ipc.h>
#include <stddef.h>
#include <kstring.h>

int32_t proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra_data, bool nonblock) {
	(void)ctx;
	_current_proc->space->ipc.entry = entry;
	_current_proc->space->ipc.extra_data = extra_data;

	/*if(nonblock) {
		proc_t *ipc_thread = kfork_raw(PROC_TYPE_IPC, _current_proc);
		if(ipc_thread == NULL)
			return -1;
		_current_proc->space->ipc.sp = ipc_thread->ctx.sp;
		_current_proc->space->ipc.ipc_pid = ipc_thread->info.pid;
		return ipc_thread->info.pid;
	}
	else {
		_current_proc->space->ipc.sp = ctx->sp;
		_current_proc->info.state = BLOCK;
	}
	*/

	if(!nonblock)
		_current_proc->info.state = BLOCK;
	return 0;
}

int32_t proc_ipc_call(context_t* ctx, proc_t* proc, ipc_t *ipc) {
	if(proc == NULL || proc->space->ipc.entry == 0 || ipc == NULL)
		return -1;
	
	ipc->proc_state = proc->info.state;
	memcpy(&ipc->ctx, &proc->ctx, sizeof(context_t));

	proc->ctx.pc = proc->ctx.lr = proc->space->ipc.entry;
	proc->ctx.gpr[0] = (uint32_t)ipc;
	proc->ctx.gpr[1] = proc->space->ipc.extra_data;
	proc->info.state = RUNNING;
	proc_switch(ctx, proc, true);
	return 0;
}
