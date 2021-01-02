#include <kernel/ipc.h>
#include <dev/gic.h>
#include <kernel/proc.h>
#include <kernel/schedule.h>
#include <stddef.h>
#include <kstring.h>

int32_t proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra_data, uint32_t flags) {
	(void)ctx;
	proc_t* cproc = get_current_proc();
	cproc->space->ipc.entry = entry;
	cproc->space->ipc.extra_data = extra_data;
	cproc->space->ipc.flags = flags;
	return 0;
}

int32_t proc_ipc_call(context_t* ctx, proc_t* proc, ipc_t *ipc) {
	proc_t* cproc = get_current_proc();
	if(proc == NULL || proc->space->ipc.entry == 0 || ipc == NULL)
		return -1;

	cproc->info.state = BLOCK;
	cproc->block_event = (uint32_t)ipc;
	cproc->info.block_by = proc->info.pid;
	proc->space->ipc.ipc = (uint32_t)ipc;
	
	if(proc->info.core == cproc->info.core) {
		proc->info.state = RUNNING;
		proc_switch(ctx, proc, true);
	}
	else {
		proc_ready(proc);
		schedule(ctx);
	}
	return 0;
}

ipc_t* proc_ipc_req(proc_t* proc) {
	return &proc->ipc_ctx;
}

void proc_ipc_close(ipc_t* ipc) {
	ipc->state = IPC_IDLE;
}
