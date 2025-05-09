#include <kernel/proc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/mmu.h>
#include <kernel/kernel.h>
#include <stddef.h>
#include <kstring.h>
#include <dev/timer.h>

#define IPC_BUFFER_SIZE 32

int32_t proc_ipc_setup(context_t* ctx, ewokos_addr_t entry, ewokos_addr_t extra_data, uint32_t flags) {
	(void)ctx;
	proc_t* cproc = get_current_proc();
	cproc->space->ipc_server.entry = entry;
	cproc->space->ipc_server.extra_data = extra_data;
	cproc->space->ipc_server.flags = flags;
	return 0;
}

inline ipc_task_t* proc_ipc_get_task(struct st_proc* serv_proc) {
	if(serv_proc->space->ipc_server.ctask.state == IPC_IDLE)
		return NULL;
	return &(serv_proc->space->ipc_server.ctask);
}

uint32_t proc_ipc_fetch(struct st_proc* serv_proc) {
	ipc_task_t* ipc = NULL;
	/*while(true) {
		ipc = (ipc_task_t*)queue_pop(&serv_proc->space->ipc_server.tasks);
		if(ipc == NULL) 
			return 0;
		proc_t* client_proc = proc_get(ipc->client_pid);
		if(client_proc != NULL && ipc->client_uuid == client_proc->info.uuid) {//available ipc
			if(client_proc->ipc_buffered > 0) {
				client_proc->ipc_buffered--;
				if(client_proc->ipc_buffered == 0) {
					if(client_proc->ipc_buffer_clean) {
						client_proc->ipc_buffer_clean = false;
						proc_wakeup(client_proc->info.pid, -1, (uint32_t)&client_proc->ipc_buffer_clean); 
					}
				}
			}
			break;
		}
		kfree(ipc); //drop unvailable ipc
	}
	*/

	ipc = proc_ipc_get_task(serv_proc);
	if(ipc == NULL)
		return 0;
	return ipc->uid;
}

int32_t proc_ipc_do_task(context_t* ctx, proc_t* serv_proc, uint32_t core) {
	ipc_task_t* ipc = proc_ipc_get_task(serv_proc);
	if(ipc == NULL ||
			ipc->state == IPC_IDLE ||
			ipc->uid == 0) {
		return -1;
	}

	proc_save_state(serv_proc, &serv_proc->space->ipc_server.saved_state, &serv_proc->space->ipc_server.saved_ipc_res);
	serv_proc->space->ipc_server.do_switch = true;

	if((ipc->call_id & IPC_LAZY) == 0)
		proc_switch_multi_core(ctx, serv_proc, core);
	return 0;
}

static void ipc_free(ipc_task_t* ipc) {
	if(ipc == NULL)
		return;
	proto_clear(&ipc->arg_ret);
	memset(ipc, 0, sizeof(ipc_task_t));
	//kfree(ipc);
}

ipc_task_t* proc_ipc_req(proc_t* serv_proc, proc_t* client_proc, int32_t call_id, proto_t* arg) {
	uint64_t usec = timer_read_sys_usec();
	ipc_task_t* ipc = &serv_proc->space->ipc_server.ctask;
	//kprintf("ipc timeout check %d\n", usec);
	if(ipc->state != IPC_IDLE) {
		return NULL;
		/*if((usec - ipc->usec) < IPC_TIMEOUT_USEC || (ipc->call_id & IPC_NON_RETURN) == 0)
			return NULL;

		kprintf("ipc timeout check %d, c: %d, s: %d, call: %d\n", (uint32_t)(usec-ipc->usec), ipc->client_pid, serv_proc->info.pid, ipc->call_id);
		if(ipc->arg_shm_id > 0) {
			kprintf("ipc timeout c: %d, s: %d, call: %d\n", ipc->client_pid, serv_proc->info.pid, ipc->call_id);
			shm_proc_unmap_by_id(serv_proc, ipc->arg_shm_id, true);
			proc_wakeup(serv_proc->info.pid, client_proc->info.pid, (uint32_t)&client_proc->ipc_res);
		}
		*/
	}
	/*
	if(client_proc->ipc_buffer_clean) //cleaning task still on
		return NULL;

	if(client_proc->ipc_buffered >= IPC_BUFFER_SIZE) {
		client_proc->ipc_buffer_clean = true;
		//kprintf("ipc buffer overflowed(%d)! c: %d, s: %d, call: %d\n", client_proc->ipc_buffered, client_proc->info.pid, serv_proc->info.pid, call_id);
		return NULL;
	}

	ipc_task_t* ipc  = (ipc_task_t*)kmalloc(sizeof(ipc_task_t));
	if(ipc == NULL)
		return NULL;
	*/
	_ipc_uid++;
	memset(ipc, 0, sizeof(ipc_task_t));
	ipc->uid = _ipc_uid;
	ipc->state = IPC_BUSY;
	ipc->client_pid = client_proc->info.pid;
	ipc->client_uuid = client_proc->info.uuid;
	ipc->call_id = call_id;
	ipc->usec = usec;
	if(arg != NULL && arg->data != NULL) {
		proto_copy(&ipc->arg_ret, arg->data, arg->size);
	}

	/*
	if(serv_proc->space->ipc_server.ctask == NULL) 
		serv_proc->space->ipc_server.ctask = ipc; //set current task
	else  {
		queue_push(&serv_proc->space->ipc_server.tasks, ipc); // buffered
		client_proc->ipc_buffered++;
	}
	*/
	return ipc; 
}

void proc_ipc_close(proc_t* serv_proc, ipc_task_t* ipc) {
	if(ipc == NULL)
		return;

	/*if(serv_proc->space->ipc_server.ctask == ipc)
		serv_proc->space->ipc_server.ctask = NULL;
	*/
	ipc_free(ipc);
}

void proc_ipc_clear(proc_t* serv_proc) {
	proc_ipc_close(serv_proc, &serv_proc->space->ipc_server.ctask);
	/*while(true) {
		ipc_task_t* ipc = (ipc_task_t*)queue_pop(&serv_proc->space->ipc_server.tasks);
		if(ipc == NULL)
			break;
		ipc_free(ipc);
	}
	*/
}