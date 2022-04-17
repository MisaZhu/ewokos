#include <kernel/ipc.h>
#include <kernel/proc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
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

uint32_t proc_ipc_fetch(struct st_proc* serv_proc) {
	ipc_task_t* ipc = &serv_proc->space->ipc_server.task;
	if(ipc->state == IPC_IDLE ||
			ipc->uid == 0) {
		ipc = (ipc_task_t*)queue_pop(&serv_proc->space->ipc_server.tasks);
		if(ipc == NULL) 
			return 0;
		memcpy(&serv_proc->space->ipc_server.task, ipc, sizeof(ipc_task_t));
		kfree(ipc);
		ipc = &serv_proc->space->ipc_server.task;
	}
	return ipc->uid;
}
 
inline ipc_task_t* proc_ipc_get_task(struct st_proc* serv_proc) {
	ipc_task_t* ipc = &serv_proc->space->ipc_server.task;
	if(ipc->state == IPC_IDLE ||
			ipc->uid == 0) {
		return NULL;
	}
	return ipc;
}

int32_t proc_ipc_do_task(context_t* ctx, proc_t* serv_proc, uint32_t core) {
	ipc_task_t* ipc = proc_ipc_get_task(serv_proc);
	if(ipc == NULL ||
			ipc->state == IPC_IDLE ||
			ipc->uid == 0) {
		return -1;
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
#ifdef KERNEL_SMP
		ipi_send(serv_proc->info.core);
#endif
	}
	return 0;
}

ipc_task_t* proc_ipc_req(proc_t* serv_proc, int32_t client_pid, int32_t call_id, proto_t* data) {
	_ipc_uid++;

	ipc_task_t* ipc = &serv_proc->space->ipc_server.task;
	if(ipc->state != IPC_IDLE) { //still busy on ipc, need to buffered
		if((call_id & IPC_NON_RETURN) == 0) //only non-return ipc can be buffered.
			return NULL;

		ipc = (ipc_task_t*)kmalloc(sizeof(ipc_task_t));
		if (ipc == NULL)
			return NULL;
		memset(ipc, 0, sizeof(ipc_task_t));
		queue_push(&serv_proc->space->ipc_server.tasks, ipc);
	}

	ipc->uid = _ipc_uid;
	ipc->state = IPC_BUSY;
	ipc->client_pid = client_pid;
	ipc->call_id = call_id;
	if(data != NULL)
		proto_copy(&ipc->data, data->data, data->size); 
	return ipc; 
}

void proc_ipc_close(proc_t* serv_proc, ipc_task_t* ipc) {
	(void)serv_proc;
	if(ipc == NULL)
		return;

	proto_clear(&ipc->data);
	memset(ipc, 0, sizeof(ipc_task_t));
	ipc->state = IPC_IDLE;
}

void proc_ipc_clear(proc_t* serv_proc) {
	proc_ipc_close(serv_proc, &serv_proc->space->ipc_server.task);
	while(true) {
		ipc_task_t* ipc = (ipc_task_t*)queue_pop(&serv_proc->space->ipc_server.tasks);
		if(ipc == NULL)
			break;
		proto_clear(&ipc->data);
		kfree(ipc);
	}
}