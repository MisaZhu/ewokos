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
#define IPC_WAKE_BATCH_LIMIT 2

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

static inline void ipc_waitq_link_tail(ipc_server_t* server, ipc_queue_item_t* item) {
    if(server == NULL || item == NULL)
        return;
    item->next = NULL;
    item->prev = server->wait_tail;
    if(server->wait_tail != NULL)
        server->wait_tail->next = item;
    else
        server->wait_head = item;
    server->wait_tail = item;
    item->queued = 1;
}

static inline void ipc_waitq_unlink(ipc_server_t* server, ipc_queue_item_t* item) {
    if(server == NULL || item == NULL || !item->queued)
        return;
    if(item->prev != NULL)
        item->prev->next = item->next;
    else
        server->wait_head = item->next;
    if(item->next != NULL)
        item->next->prev = item->prev;
    else
        server->wait_tail = item->prev;
    item->next = NULL;
    item->prev = NULL;
    item->queued = 0;
}

static inline ipc_queue_item_t* ipc_waitq_pop(ipc_server_t* server) {
    ipc_queue_item_t* item = NULL;
    if(server == NULL)
        return NULL;
    item = server->wait_head;
    if(item == NULL)
        return NULL;
    if(item->next != NULL)
        item->next->prev = NULL;
    else
        server->wait_tail = NULL;
    server->wait_head = item->next;
    item->next = NULL;
    item->prev = NULL;
    item->queued = 0;
    return item;
}

static inline void ipc_wait_item_set_server(ipc_queue_item_t* item, proc_t* serv_proc) {
    if(item == NULL || item->owner == NULL)
        return;
    if(item->owner->info.uuid == item->uuid &&
            item->owner->ipc_wait_item.uuid == item->uuid) {
        item->owner->ipc_waiting_on = serv_proc;
    }
}

static inline void ipc_wait_item_clear_server(ipc_queue_item_t* item, proc_t* serv_proc) {
    if(item == NULL || item->owner == NULL)
        return;
    /*
     * Mirror proc_ipc_cancel_wait(): the queue item is embedded in
     * item->owner itself, so once this exact item is popped from
     * serv_proc's wait list we must clear owner->ipc_waiting_on
     * regardless of the owner's CURRENT uuid.
     *
     * exec() reassigns info.uuid in-place. Keeping the old uuid gate here
     * leaves owner->ipc_waiting_on pointing at serv_proc after the waiter
     * was already dequeued. A later proc_ipc_cancel_wait() on another IPC
     * path then follows that stale raw proc_t* - potentially after the
     * old server proc was buried - and dereferences freed/reused memory.
     */
    if(item->owner->ipc_waiting_on == serv_proc) {
        item->owner->ipc_waiting_on = NULL;
    }
}

static inline bool ipc_taskq_is_empty(const ipc_server_t* server) {
    return (server == NULL || server->task_num == 0);
}

static inline bool ipc_taskq_is_full(const ipc_server_t* server) {
    return (server != NULL && server->task_num >= IPC_CTX_MAX);
}

static inline ipc_task_t* ipc_taskq_head(ipc_server_t* server) {
    if(ipc_taskq_is_empty(server))
        return NULL;
    return &server->tasks[server->task_head];
}

static inline ipc_task_t* ipc_taskq_tail_slot(ipc_server_t* server) {
    if(server == NULL || ipc_taskq_is_full(server))
        return NULL;
    return &server->tasks[server->task_tail];
}

static inline void ipc_taskq_push_tail(ipc_server_t* server) {
    if(server == NULL || ipc_taskq_is_full(server))
        return;
    server->task_tail = (uint8_t)((server->task_tail + 1) % IPC_CTX_MAX);
    server->task_num++;
}

static inline void ipc_taskq_pop_head(ipc_server_t* server) {
    if(ipc_taskq_is_empty(server))
        return;
    server->task_head = (uint8_t)((server->task_head + 1) % IPC_CTX_MAX);
    server->task_num--;
    if(server->task_num == 0) {
        server->task_head = 0;
        server->task_tail = 0;
    }
}

static inline bool ipc_taskq_is_head_slot(const ipc_server_t* server, const ipc_task_t* ipc) {
    if(server == NULL || ipc == NULL || ipc_taskq_is_empty(server))
        return false;
    return (&server->tasks[server->task_head] == ipc);
}

static inline void ipc_task_reset(ipc_task_t* ipc) {
    if(ipc == NULL)
        return;
    proto_clear(&ipc->arg_ret);
    ipc->uid = 0;
    ipc->counter = 0;
    ipc->state = IPC_IDLE;
    ipc->client_pid = 0;
    ipc->client_uuid = 0;
    ipc->client_intr = 0;
    ipc->call_id = 0;
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
    return ipc_taskq_head(&serv_proc->space->ipc_server);
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
    ipc_task_reset(ipc);
}

ipc_task_t* proc_ipc_req(proc_t* serv_proc, proc_t* client_proc, int32_t call_id, proto_t* arg) {
    ipc_server_t* server = &serv_proc->space->ipc_server;
    ipc_task_t* ipc = NULL;
    //kprintf("ipc timeout check %d\n", usec);

    proc_ipc_server_lock(server);

    if(ipc_taskq_is_full(server)) {
        proc_ipc_server_unlock(server);
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
    ipc = ipc_taskq_tail_slot(server);
    if(ipc == NULL) {
        proc_ipc_server_unlock(server);
        return NULL;
    }

    _ipc_uid++;
    ipc_task_reset(ipc);
    ipc->uid = _ipc_uid;
    ipc->state = IPC_BUSY;
    ipc->client_pid = client_proc->info.pid;
    ipc->client_uuid = client_proc->info.uuid;
    ipc->client_intr = (client_proc->space != NULL &&
            client_proc->space->interrupt.state == INTR_STATE_WORKING) ? 1 : 0;
    ipc->call_id = call_id;
    ipc->counter = 0;
    if(arg != NULL && arg->data != NULL) {
        proto_copy(&ipc->arg_ret, arg->data, arg->size);
    }
    ipc_taskq_push_tail(server);
    proc_ipc_server_unlock(server);
    proc_track_ipc_timeout(serv_proc);
    return ipc; 
}

void proc_ipc_close(proc_t* serv_proc, ipc_task_t* ipc) {
    ipc_server_t* server = NULL;
    if(serv_proc == NULL || ipc == NULL || serv_proc->space == NULL)
        return;
    server = &serv_proc->space->ipc_server;
    proc_untrack_ipc_timeout(serv_proc);
    proc_ipc_server_lock(server);
    if(ipc_taskq_is_head_slot(server, ipc)) {
        ipc_free(ipc);
        ipc_taskq_pop_head(server);
    }
    proc_ipc_server_unlock(server);
    if(proc_ipc_get_task(serv_proc) != NULL)
        proc_track_ipc_timeout(serv_proc);
}

void proc_ipc_clear(proc_t* serv_proc) {
    if(serv_proc == NULL || serv_proc->space == NULL)
        return;
    ipc_server_t* server = &serv_proc->space->ipc_server;
    proc_untrack_ipc_timeout(serv_proc);
    proc_ipc_server_lock(server);
    while(!ipc_taskq_is_empty(server)) {
        ipc_task_t* ipc = ipc_taskq_head(server);
        ipc_free(ipc);
        ipc_taskq_pop_head(server);
    }
    proc_ipc_server_unlock(server);
}

int32_t proc_ipc_wait(context_t* ctx, struct st_proc* serv_proc, proc_t* proc) {
    if(serv_proc == NULL || proc == NULL)
        return -1;

    if(proc->ipc_waiting_on != NULL && proc->ipc_waiting_on != serv_proc)
        proc_ipc_cancel_wait(proc);

    ipc_queue_item_t* item = &proc->ipc_wait_item;
    item->owner = proc;
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
            !ipc_taskq_is_full(&serv_proc->space->ipc_server)) {
        proc_ipc_server_unlock(&serv_proc->space->ipc_server);
        return 1;
    }
    if(!item->queued) {
        ipc_waitq_link_tail(&serv_proc->space->ipc_server, item);
        ipc_wait_item_set_server(item, serv_proc);
    }
    proc_ipc_server_unlock(&serv_proc->space->ipc_server);
    proc_block(ctx, proc);
    return 0;
}

void proc_ipc_cancel_wait(struct st_proc* proc) {
    if(proc == NULL || proc->ipc_waiting_on == NULL)
        return;
    proc_t* serv_proc = proc->ipc_waiting_on;
    if(serv_proc->space == NULL)
        return;
    ipc_queue_item_t* item = &proc->ipc_wait_item;
    proc_ipc_server_lock(&serv_proc->space->ipc_server);
    /*
     * The item is embedded in this very proc_t, so owner==proc by
     * construction. Do NOT gate the unlink on the uuid still matching:
     * exec() reassigns the uuid while a spuriously-woken waiter can still
     * be queued, and skipping the unlink then leaves the item linked in
     * the server queue with no handle left to ever remove it - a dangling
     * pointer into freed memory once the proc is buried.
     */
    if(item->queued) {
        ipc_waitq_unlink(&serv_proc->space->ipc_server, item);
    }
    if(proc->ipc_waiting_on == serv_proc)
        proc->ipc_waiting_on = NULL;
    proc_ipc_server_unlock(&serv_proc->space->ipc_server);
}

proc_t* proc_ipc_wakeup(struct st_proc* serv_proc) {
    proc_t* first_woken = NULL;
    uint32_t wake_budget = 0;
    if(serv_proc == NULL)
        return NULL;

    proc_ipc_server_lock(&serv_proc->space->ipc_server);
    wake_budget = (uint32_t)(IPC_CTX_MAX - serv_proc->space->ipc_server.task_num);
    if(wake_budget > IPC_WAKE_BATCH_LIMIT)
        wake_budget = IPC_WAKE_BATCH_LIMIT;
    proc_ipc_server_unlock(&serv_proc->space->ipc_server);

    while(wake_budget > 0) {
        proc_ipc_server_lock(&serv_proc->space->ipc_server);
        ipc_queue_item_t* item = ipc_waitq_pop(&serv_proc->space->ipc_server);
        ipc_wait_item_clear_server(item, serv_proc);
        proc_ipc_server_unlock(&serv_proc->space->ipc_server);
        if(item == NULL)
            break;

        proc_t* proc = proc_get(item->pid);
        uint32_t waiter_uuid = item->uuid;

        if(proc == NULL || proc->info.uuid != waiter_uuid)
            continue;

        /*
         * Skip stale waiters that were already released by another wake
         * edge. Stopping after popping one such item leaves real blocked
         * waiters stranded behind it even though the server is idle.
         */
        if(proc->info.state != BLOCK &&
                        proc->info.state != WAIT &&
                        proc->info.state != SLEEPING) {
            continue;
        }

        proc_wakeup(proc);
        if(first_woken == NULL)
            first_woken = proc;
        wake_budget--;
    }
    return first_woken;
}

void proc_ipc_wakeup_all(struct st_proc* serv_proc) {
    if(serv_proc == NULL)
        return;

    while(true) {
        proc_ipc_server_lock(&serv_proc->space->ipc_server);
        ipc_queue_item_t* item = ipc_waitq_pop(&serv_proc->space->ipc_server);
        ipc_wait_item_clear_server(item, serv_proc);
        proc_ipc_server_unlock(&serv_proc->space->ipc_server);
        if(item == NULL)
            break;
        proc_t* proc = proc_get(item->pid);
        if(proc == NULL || proc->info.uuid != item->uuid) {
            continue;
        }
        proc_wakeup(proc);
    }
}
