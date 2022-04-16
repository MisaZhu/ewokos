#include <kernel/ipc.h>
#include <kernel/proc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <mm/kalloc.h>
#include <mm/mmu.h>
#include <stddef.h>
#include <kstring.h>
#include <dev/ipi.h>

int32_t proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra_data, uint32_t flags) {
	(void)ctx;
	proc_t* cproc = get_current_proc();
	cproc->space->ipc_server.entry = entry;
	cproc->space->ipc_server.extra_data = extra_data;
	cproc->space->ipc_server.flags = flags;
	
	if(cproc->space->inter_stack == 0) {
		uint32_t page = (uint32_t)kalloc4k();
		map_page(cproc->space->vm, page, V2P(page), AP_RW_RW, 0);
		cproc->space->inter_stack = page;
		flush_tlb();
	}
	return 0;
}

ipc_task_t* proc_ipc_fetch(struct st_proc* serv_proc) {
	return &serv_proc->space->ipc_server.task;
}

void proc_ipc_do_task(context_t* ctx, proc_t* serv_proc, uint32_t core) {
	ipc_task_t* ipc = proc_ipc_fetch(serv_proc);
	if(ipc == NULL ||
			ipc->state == IPC_IDLE ||
			ipc->uid == 0) {
		return;
	}

	ipc_context_t* ipc_ctx = &serv_proc->space->ipc_server.ctx;
	ipc_ctx->saved_state = serv_proc->info.state;
	ipc_ctx->saved_block_by = serv_proc->info.block_by;
	ipc_ctx->saved_block_event = serv_proc->block_event;
	serv_proc->space->ipc_server.start = true;

	if(serv_proc->info.core == core) {
		serv_proc->info.state = RUNNING;
		proc_switch(ctx, serv_proc, true);
	}
	else {
		proc_ready(serv_proc);
		schedule(ctx);
#ifdef KERNEL_SMP
		ipi_send(serv_proc->info.core);
#endif
	}
}

uint32_t proc_ipc_req(proc_t* serv_proc, int32_t client_pid, int32_t call_id, proto_t* data) {
	ipc_task_t* ipc = &serv_proc->space->ipc_server.task;
	ipc->uid = ++_ipc_uid;
	ipc->state = IPC_BUSY;
	ipc->client_pid = client_pid;
	ipc->call_id = call_id;
	if(data != NULL)
		proto_copy(&ipc->data, data->data, data->size); 
	return ipc->uid;
}

void proc_ipc_close(ipc_task_t* ipc) {
	proto_clear(&ipc->data);
	memset(ipc, 0, sizeof(ipc_task_t));
	ipc->state = IPC_IDLE;
}