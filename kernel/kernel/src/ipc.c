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

#ifdef KERNEL_SMP
extern void mcore_lock(int32_t* v);
extern void mcore_unlock(int32_t* v);
#endif

#define IPC_BUFFER_SIZE 32

static inline void proc_ipc_server_lock(ipc_server_t* server) {
#ifdef KERNEL_SMP
	if(server != NULL)
		mcore_lock(&server->lock);
#else
	(void)server;
#endif
}

static inline void proc_ipc_server_unlock(ipc_server_t* server) {
#ifdef KERNEL_SMP
	if(server != NULL)
		mcore_unlock(&server->lock);
#else
	(void)server;
#endif
}

int32_t proc_ipc_setup(context_t* ctx, ewokos_addr_t entry, ewokos_addr_t extra_data, uint32_t flags) {
	(void)ctx;
	proc_t* cproc = get_current_proc();
	cproc->space->ipc_server.entry = entry;
	cproc->space->ipc_server.extra_data = extra_data;
	cproc->space->ipc_server.flags = flags;
	return 0;
}

inline ipc_task_t* proc_ipc_get_task(struct st_proc* serv_proc) {
	if(serv_proc == NULL || serv_proc->space == NULL)
		return NULL;
	if(serv_proc->space->ipc_server.ctask.state == IPC_IDLE)
		return NULL;
	return &(serv_proc->space->ipc_server.ctask);
}

uint32_t proc_ipc_fetch(struct st_proc* serv_proc) {
	ipc_task_t* ipc = NULL;

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

	proc_lock_enter();
	proc_save_state(serv_proc, &serv_proc->space->ipc_server.saved_state, &serv_proc->space->ipc_server.saved_ipc_res);
	serv_proc->space->ipc_server.do_switch = true;
	serv_proc->space->ipc_server.restore_pending = 0;
	proc_lock_leave();

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
	ipc_task_t* ipc = &serv_proc->space->ipc_server.ctask;
	//kprintf("ipc timeout check %d\n", usec);

	proc_ipc_server_lock(&serv_proc->space->ipc_server);

	if(ipc->state != IPC_IDLE) {
		proc_ipc_server_unlock(&serv_proc->space->ipc_server);
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
	ipc->client_intr = (client_proc->space != NULL &&
			client_proc->space->interrupt.state == INTR_STATE_WORKING) ? 1 : 0;
	ipc->call_id = call_id;
	ipc->counter = 0;

	proc_ipc_server_unlock(&serv_proc->space->ipc_server);

	if(arg != NULL && arg->data != NULL) {
		proto_copy(&ipc->arg_ret, arg->data, arg->size);
	}
	proc_track_ipc_timeout(serv_proc);
	return ipc; 
}

void proc_ipc_close(proc_t* serv_proc, ipc_task_t* ipc) {
	if(ipc == NULL)
		return;
	proc_untrack_ipc_timeout(serv_proc);

	/*if(serv_proc->space->ipc_server.ctask == ipc)
		serv_proc->space->ipc_server.ctask = NULL;
	*/
	ipc_free(ipc);
}

void proc_ipc_clear(proc_t* serv_proc) {
	proc_ipc_close(serv_proc, &serv_proc->space->ipc_server.ctask);
}

int32_t proc_ipc_wait(context_t* ctx, struct st_proc* serv_proc, proc_t* proc) {
	if(serv_proc == NULL || proc == NULL)
		return -1;

	ipc_queue_item_t* item = (ipc_queue_item_t*)kmalloc(sizeof(ipc_queue_item_t));
	if(item == NULL)
		return -1;

	item->pid = proc->info.pid;
	item->uuid = proc->info.uuid;
	proc_ipc_server_lock(&serv_proc->space->ipc_server);
	/*
	 * Re-check under the same server lock used by proc_ipc_req()/wakeup().
	 * Otherwise sys_ipc_call() can observe "busy", then the server finishes
	 * and drains waiters before we enqueue ourselves, losing that wake edge
	 * and sleeping until IPC timeout recovery fires.
	 */
	if(!serv_proc->space->ipc_server.disabled &&
			serv_proc->space->ipc_server.ctask.state == IPC_IDLE) {
		proc_ipc_server_unlock(&serv_proc->space->ipc_server);
		kfree(item);
		return 1;
	}
	/*
	 * Dedup: a proc may re-enter proc_ipc_wait() after being released by a
	 * spurious/generic wake (signal, token-0 IPC-return edge, etc) while its
	 * previous wait item is STILL queued (that item was never popped). Pushing
	 * a second item lets proc_ipc_wakeup() waste a pop on the stale duplicate
	 * (the proc is already READY), stranding genuine waiters behind it - the
	 * server goes idle with a non-empty wait_queue and the reader blocks until
	 * the peer closes. Reuse the existing item instead: at most one item per
	 * pid, so every pop wakes a genuinely-waiting proc.
	 */
	bool already_queued = false;
	{
		queue_item_t* qit = serv_proc->space->ipc_server.wait_queue.head;
		while(qit != NULL) {
			ipc_queue_item_t* qi = (ipc_queue_item_t*)qit->data;
			if(qi != NULL && qi->pid == proc->info.pid &&
					qi->uuid == proc->info.uuid) {
				already_queued = true;
				break;
			}
			qit = qit->next;
		}
	}
	if(!already_queued)
		queue_push(&serv_proc->space->ipc_server.wait_queue, item);
	proc_ipc_server_unlock(&serv_proc->space->ipc_server);
	if(already_queued)
		kfree(item);
	proc_block(ctx, proc);
	return 0;
}

proc_t* proc_ipc_wakeup(struct st_proc* serv_proc) {
	if(serv_proc == NULL)
		return NULL;

	proc_ipc_server_lock(&serv_proc->space->ipc_server);
	ipc_queue_item_t* item = (ipc_queue_item_t*)queue_pop(&serv_proc->space->ipc_server.wait_queue);
	proc_ipc_server_unlock(&serv_proc->space->ipc_server);
	if(item == NULL)
		return NULL;
	//printf("ipc ipc_wakeup %d %d\n", item->pid, item->uuid);
	proc_t* proc = proc_get(item->pid);
	if(proc == NULL || proc->info.uuid != item->uuid) {
		kfree(item);
		return NULL;
	}
	kfree(item);
	proc_wakeup(proc);
	return proc;
}

void proc_ipc_wakeup_all(struct st_proc* serv_proc) {
	if(serv_proc == NULL)
		return;

	while(true) {
		proc_ipc_server_lock(&serv_proc->space->ipc_server);
		ipc_queue_item_t* item = (ipc_queue_item_t*)queue_pop(&serv_proc->space->ipc_server.wait_queue);
		proc_ipc_server_unlock(&serv_proc->space->ipc_server);
		if(item == NULL)
			break;
		proc_t* proc = proc_get(item->pid);
		if(proc == NULL || proc->info.uuid != item->uuid) {
			kfree(item);
			continue;
		}
		kfree(item);
		proc_wakeup(proc);
	}
}
