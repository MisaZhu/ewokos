#include <kernel/proc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/mmu.h>
#include <stddef.h>
#include <kstring.h>

int32_t proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra_data, uint32_t flags) {
	(void)ctx;
	proc_t* cproc = get_current_proc();
	cproc->space->ipc_server.entry = entry;
	cproc->space->ipc_server.extra_data = extra_data;
	cproc->space->ipc_server.flags = flags;
	
	cproc->space->ipc_server.stack = proc_stack_alloc(cproc);
	return 0;
}

uint32_t proc_ipc_fetch(struct st_proc* serv_proc) {
	ipc_task_t* ipc = (ipc_task_t*)queue_pop(&serv_proc->space->ipc_server.tasks);
	if(ipc == NULL) 
		return 0;
	serv_proc->space->ipc_server.ctask = ipc;
	return ipc->uid;
}
 
inline ipc_task_t* proc_ipc_get_task(struct st_proc* serv_proc) {
	return serv_proc->space->ipc_server.ctask;
}

int32_t proc_ipc_do_task(context_t* ctx, proc_t* serv_proc, uint32_t core) {
	ipc_task_t* ipc = proc_ipc_get_task(serv_proc);
	if(ipc == NULL ||
			ipc->state == IPC_IDLE ||
			ipc->uid == 0) {
		return -1;
	}

	proc_save_state(serv_proc, &serv_proc->space->ipc_server.saved_state);
	serv_proc->space->ipc_server.do_switch = true;

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
	ipc_task_t* ipc  = (ipc_task_t*)kmalloc(sizeof(ipc_task_t));
	if(ipc == NULL)
		return NULL;

	memset(ipc, 0, sizeof(ipc_task_t));
	proto_init(&ipc->data);
	ipc->uid = _ipc_uid;
	ipc->state = IPC_BUSY;
	ipc->client_pid = client_pid;
	ipc->call_id = call_id;
	if(data != NULL) {
		proto_copy(&ipc->data, data->data, data->size); 
	}

	if(serv_proc->space->ipc_server.ctask == NULL) 
		serv_proc->space->ipc_server.ctask = ipc; //set current task
	else
		queue_push(&serv_proc->space->ipc_server.tasks, ipc); // buffered
	return ipc; 
}

void proc_ipc_close(proc_t* serv_proc, ipc_task_t* ipc) {
	(void)serv_proc;
	if(ipc == NULL)
		return;

	proto_clear(&ipc->data);
	if(serv_proc->space->ipc_server.ctask == ipc)
		serv_proc->space->ipc_server.ctask = NULL;
	kfree(ipc);
}

void proc_ipc_clear(proc_t* serv_proc) {
	proc_ipc_close(serv_proc, serv_proc->space->ipc_server.ctask);
	while(true) {
		ipc_task_t* ipc = (ipc_task_t*)queue_pop(&serv_proc->space->ipc_server.tasks);
		if(ipc == NULL)
			break;
		proto_clear(&ipc->data);
		kfree(ipc);
	}
}