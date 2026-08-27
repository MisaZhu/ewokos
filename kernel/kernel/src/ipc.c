#include <kernel/proc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <kernel/interrupt.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/mmu.h>
#include <kernel/kernel.h>
#include <stddef.h>
#include <kstring.h>
#include <kprintf.h>
#include <syscalls.h>
#include <dev/timer.h>

#define IPC_BUFFER_SIZE 32
#define IPC_WAKE_BATCH_LIMIT 2

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
    ipc->handler_pid = 0;
    ipc->handler_uuid = 0;
}

int32_t proc_ipc_setup(context_t* ctx, ewokos_addr_t entry, ewokos_addr_t extra_data, uint32_t flags) {
    (void)ctx;
    proc_t* cproc = get_current_proc();
    cproc->space->ipc_server.entry = entry;
    cproc->space->ipc_server.extra_data = extra_data;
    cproc->space->ipc_server.flags = flags;
    /*
     * IPC_MULTI_TASK: the server proc itself never executes ipc handlers.
     * Every incoming request is served by a kernel-spawned worker thread
     * inside this proc, so requests run concurrently and the server's main
     * context is left untouched.
     */
    cproc->space->ipc_server.multi_task = ((flags & IPC_MULTI_TASK) != 0);
    return 0;
}

inline ipc_task_t* proc_ipc_get_task(struct st_proc* serv_proc) {
    if(serv_proc == NULL || serv_proc->space == NULL)
        return NULL;
    return ipc_taskq_head(&serv_proc->space->ipc_server);
}

uint32_t proc_ipc_task_count(struct st_proc* serv_proc) {
    if(serv_proc == NULL || serv_proc->space == NULL)
        return 0;
    return serv_proc->space->ipc_server.task_num;
}

/*
 * Find an in-flight ipc task of the server by its uid. Needed by
 * multi_task servers where tasks complete out of order, so the queue head
 * (proc_ipc_get_task) no longer identifies a specific request.
 */
ipc_task_t* proc_ipc_find_task(struct st_proc* serv_proc, uint32_t uid) {
    if(serv_proc == NULL || serv_proc->space == NULL || uid == 0)
        return NULL;
    ipc_server_t* server = &serv_proc->space->ipc_server;
    for(uint32_t i = 0; i < IPC_CTX_MAX; i++) {
        ipc_task_t* ipc = &server->tasks[i];
        if(ipc->uid == uid && ipc->state != IPC_IDLE)
            return ipc;
    }
    return NULL;
}

/*
 * The ipc task the CURRENT execution context is serving. In single-task
 * mode that is the owner proc's queue head (the main context is the only
 * handler); in multi_task mode only the worker thread spawned for a
 * request serves it, recorded in proc->ipc_task.
 */
ipc_task_t* proc_ipc_current_task(struct st_proc* proc) {
    if(proc == NULL || proc->space == NULL)
        return NULL;
    proc_t* owner = proc_get_proc(proc);
    if(owner == NULL || owner->space == NULL)
        return NULL;
    if(owner->space->ipc_server.multi_task)
        return proc->ipc_task;
    return proc_ipc_get_task(owner);
}

/*
 * Resolve and validate the ipc task this context may operate on for the
 * given uid (SYS_IPC_GET_ARG / SYS_IPC_SET_RETURN / SYS_IPC_END).
 */
ipc_task_t* proc_ipc_serving_task(struct st_proc* proc, uint32_t uid) {
    if(proc == NULL || uid == 0)
        return NULL;
    ipc_task_t* ipc = proc_ipc_current_task(proc);
    if(ipc == NULL ||
            ipc->uid != uid ||
            ipc->state != IPC_BUSY)
        return NULL;
    return ipc;
}

/*
 * True when this context is synchronously serving an ipc request, which
 * only happens in single-task mode where the owner proc's main context is
 * hijacked to run the handler. Such a context must not enter real sleeps
 * or blocks - the saved-state restore machine would be stranded. In
 * multi_task mode requests run in independent worker threads, so this is
 * always false.
 */
bool proc_ipc_sync_serving(struct st_proc* proc) {
    if(proc == NULL || proc->space == NULL)
        return false;
    proc_t* owner = proc_get_proc(proc);
    if(owner == NULL || owner->space == NULL ||
            owner->space->ipc_server.multi_task)
        return false;
    return (proc_ipc_get_task(owner) != NULL);
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
    if(server->multi_task) {
        /*
         * multi_task servers serve requests concurrently in worker threads,
         * so tasks complete out of order. Use the slots as a plain pool:
         * grab any free one instead of the FIFO ring tail.
         */
        for(uint32_t i = 0; i < IPC_CTX_MAX; i++) {
            if(server->tasks[i].uid == 0) {
                ipc = &server->tasks[i];
                break;
            }
        }
        if(ipc == NULL) {
            proc_ipc_server_unlock(server);
            return NULL;
        }
    }
    else {
        ipc = ipc_taskq_tail_slot(server);
        if(ipc == NULL) {
            proc_ipc_server_unlock(server);
            return NULL;
        }
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
    if(server->multi_task)
        server->task_num++;
    else
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
    if(server->multi_task) {
        /* out-of-order completion: free this exact slot */
        if(ipc->uid != 0) {
            ipc_free(ipc);
            if(server->task_num > 0)
                server->task_num--;
        }
    }
    else if(ipc_taskq_is_head_slot(server, ipc)) {
        ipc_free(ipc);
        ipc_taskq_pop_head(server);
    }
    proc_ipc_server_unlock(server);
    if(proc_ipc_task_count(serv_proc) > 0)
        proc_track_ipc_timeout(serv_proc);
}

/*
 * Complete a multi_task request from SYS_IPC_END. Claims the task and
 * resolves its client atomically under the server lock, so a concurrent
 * watchdog abort (timer context, another core) can never leave this path
 * reading a slot that was already freed and reused. Returns the client to
 * wake (NULL if the task was already aborted) and reports through
 * *wake_client whether that client waits for a return at all.
 */
proc_t* proc_ipc_finish_task(proc_t* serv_proc, ipc_task_t* ipc, bool* wake_client) {
    proc_t* client_proc = NULL;
    if(serv_proc == NULL || ipc == NULL || serv_proc->space == NULL || wake_client == NULL)
        return NULL;
    ipc_server_t* server = &serv_proc->space->ipc_server;
    if(!server->multi_task)
        return NULL;

    *wake_client = false;
    proc_untrack_ipc_timeout(serv_proc);
    proc_ipc_server_lock(server);
    if(ipc->uid != 0 && ipc->state != IPC_IDLE) {
        *wake_client = ((ipc->call_id & IPC_NON_RETURN) == 0);
        client_proc = proc_ipc_get_client(ipc);
        ipc_free(ipc);
        if(server->task_num > 0)
            server->task_num--;
    }
    proc_ipc_server_unlock(server);
    if(proc_ipc_task_count(serv_proc) > 0)
        proc_track_ipc_timeout(serv_proc);
    return client_proc;
}

/*
 * Abort an in-flight multi_task ipc request: reset the client's reply slot
 * (so the client's ipc_call fails instead of waiting forever), free the
 * task slot and wake the client. Used when a worker thread dies without
 * finishing the request or when the watchdog times the request out.
 */
void proc_ipc_task_abort(proc_t* serv_proc, ipc_task_t* ipc) {
    if(serv_proc == NULL || ipc == NULL || serv_proc->space == NULL)
        return;
    ipc_server_t* server = &serv_proc->space->ipc_server;
    if(!server->multi_task)
        return;

    proc_ipc_server_lock(server);
    if(ipc->uid == 0 || ipc->state == IPC_IDLE) {
        proc_ipc_server_unlock(server);
        return;
    }

    uint32_t uid = ipc->uid;
    /*
     * Detach the worker thread from this task before freeing the slot: a
     * timed-out or abandoned worker may still reach SYS_IPC_SET_RETURN /
     * SYS_IPC_END later, and must see "no task" instead of a slot that may
     * already serve a different request.
     */
    proc_t* handler = proc_get(ipc->handler_pid);
    if(handler != NULL &&
            handler->info.uuid == ipc->handler_uuid &&
            handler->ipc_task == ipc) {
        handler->ipc_task = NULL;
    }

    proc_t* client_proc = proc_ipc_get_client(ipc);
    ipc_res_t* cres = proc_ipc_client_res(client_proc, ipc);
    if(cres != NULL && cres->uid == uid) {
        cres->uid = 0;
        cres->state = IPC_IDLE;
        proto_clear(&cres->data);
    }

    ipc_free(ipc);
    if(server->task_num > 0)
        server->task_num--;
    proc_ipc_server_unlock(server);

    if(client_proc != NULL &&
            client_proc->info.state != UNUSED &&
            client_proc->info.state != ZOMBIE) {
        proc_wakeup(client_proc);
    }
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

/* ======================================================================
 * multi_task worker pool: a server starts with IPC_TASK_POOL_MIN_NUM
 * persistent threads (spawned parked/BLOCK on first use) and grows on
 * demand up to IPC_TASK_POOL_MAX_NUM while every member is busy; only
 * at the maximum with all members busy does the client block. Members
 * are woken with a rewritten context per request and re-parked after
 * SYS_IPC_END instead of being created/terminated for every request.
 * ====================================================================== */

/*
 * Resolve pool slot i to its live worker thread. Drops stale slots (the
 * worker died or its pid slot was reused) so replenish can refill them.
 * Caller holds the server lock (only needed for the stale-slot write).
 */
static proc_t* ipc_pool_slot_worker(ipc_server_t* server, uint32_t i) {
    if(server->pool[i].pid <= 0)
        return NULL;
    proc_t* w = proc_get(server->pool[i].pid);
    if(w == NULL ||
            w->info.uuid != server->pool[i].uuid ||
            w->info.type != TASK_TYPE_THREAD) {
        server->pool[i].pid = 0;
        server->pool[i].uuid = 0;
        server->pool[i].idle_sec = 0;
        server->pool[i].quit = 0;
        return NULL;
    }
    return w;
}

static bool ipc_pool_is_member(ipc_server_t* server, proc_t* worker) {
    for(uint32_t i = 0; i < IPC_TASK_POOL_MAX_NUM; i++) {
        if(server->pool[i].pid == worker->info.pid &&
                server->pool[i].uuid == worker->info.uuid)
            return true;
    }
    return false;
}

static bool ipc_pool_has_idle(ipc_server_t* server) {
    for(uint32_t i = 0; i < IPC_TASK_POOL_MAX_NUM; i++) {
        proc_t* w = ipc_pool_slot_worker(server, i);
        if(w != NULL && !server->pool[i].quit &&
                w->info.state == BLOCK && w->ipc_task == NULL)
            return true;
    }
    return false;
}

/*
 * Number of live pool members / register a worker in a free slot.
 * Caller holds the server lock for both. Registration failure is
 * defensive only (syscalls are serialized by kernel_lock, so a slot
 * counted free stays free); an unregistered worker still serves the one
 * request it gets and then terminates via the non-member ipc_end path.
 */
static uint32_t ipc_pool_count_locked(ipc_server_t* server) {
    uint32_t num = 0;
    for(uint32_t i = 0; i < IPC_TASK_POOL_MAX_NUM; i++) {
        if(ipc_pool_slot_worker(server, i) != NULL)
            num++;
    }
    return num;
}

static bool ipc_pool_register_locked(ipc_server_t* server, proc_t* worker) {
    for(uint32_t i = 0; i < IPC_TASK_POOL_MAX_NUM; i++) {
        if(ipc_pool_slot_worker(server, i) == NULL) {
            server->pool[i].pid = worker->info.pid;
            server->pool[i].uuid = worker->info.uuid;
            server->pool[i].idle_sec = 0;
            server->pool[i].quit = 0;
            return true;
        }
    }
    return false;
}

/*
 * Bind request ipc to a parked worker: rewrite its saved context so it
 * (re-)enters the server's ipc entry (arg0 = ipc uid, arg1 = extra_data)
 * on a fresh stack. proc_switch() restores exactly this context when the
 * scheduler picks the woken worker. Caller holds the server lock.
 */
static void ipc_pool_bind_locked(ipc_server_t* server, proc_t* worker, ipc_task_t* ipc) {
    /* the worker stops being idle: drop its park timestamp/quit mark */
    for(uint32_t i = 0; i < IPC_TASK_POOL_MAX_NUM; i++) {
        if(server->pool[i].pid == worker->info.pid &&
                server->pool[i].uuid == worker->info.uuid) {
            server->pool[i].idle_sec = 0;
            server->pool[i].quit = 0;
            break;
        }
    }
    worker->ipc_task = ipc;
    ipc->handler_pid = worker->info.pid;
    ipc->handler_uuid = worker->info.uuid;
    worker->ctx.pc = server->entry;
    worker->ctx.lr = server->entry;
    worker->ctx.gpr[0] = ipc->uid;
    worker->ctx.gpr[1] = server->extra_data;
    worker->ctx.sp = ALIGN_DOWN(worker->thread_stack_base +
            THREAD_STACK_PAGES*PAGE_SIZE, EWOK_STACK_ALIGN) - EWOK_STACK_INIT_BIAS;
}

/*
 * Ensure the pool holds at least IPC_TASK_POOL_MIN_NUM live workers
 * (spawned parked). The pool is NOT filled to the maximum up front:
 * additional members up to IPC_TASK_POOL_MAX_NUM are created on demand
 * by ipc_pool_assign() when every current member is busy. Spawn failures
 * (task table or thread stack slots exhausted) leave the pool short; the
 * next ipc call retries. Runs in syscall context (kernel_lock held), so
 * no two replenishers can race for the same slot.
 */
static void ipc_pool_replenish(proc_t* serv_proc) {
    ipc_server_t* server = &serv_proc->space->ipc_server;
    while(true) {
        proc_ipc_server_lock(server);
        bool missing = (ipc_pool_count_locked(server) < IPC_TASK_POOL_MIN_NUM);
        proc_ipc_server_unlock(server);
        if(!missing)
            return;
        proc_t* w = proc_ipc_pool_spawn(serv_proc);
        if(w == NULL)
            return; //resources exhausted; retry on a later call
        proc_ipc_server_lock(server);
        ipc_pool_register_locked(server, w);
        proc_ipc_server_unlock(server);
    }
}

/*
 * Hand request ipc to a pool worker. First tries a parked/idle member;
 * when all current members are busy it grows the pool on demand (spawn
 * parked + register + bind) up to IPC_TASK_POOL_MAX_NUM. Returns the
 * bound worker, or NULL only when the pool is at its maximum and every
 * member is busy - the caller then blocks the client until one parks.
 */
static proc_t* ipc_pool_assign(proc_t* serv_proc, ipc_task_t* ipc) {
    ipc_server_t* server = &serv_proc->space->ipc_server;
    proc_t* worker = NULL;

    proc_ipc_server_lock(server);
    for(uint32_t i = 0; i < IPC_TASK_POOL_MAX_NUM; i++) {
        proc_t* w = ipc_pool_slot_worker(server, i);
        if(w == NULL || server->pool[i].quit ||
                w->info.state != BLOCK || w->ipc_task != NULL)
            continue;
        worker = w;
        ipc_pool_bind_locked(server, worker, ipc);
        break;
    }
    if(worker == NULL) {
        /*
         * No idle member: grow the pool on demand. The count check is
         * under the lock; the spawn runs outside it (it can bury zombies)
         * and the new worker is registered + bound in the next critical
         * section. Syscalls are serialized by kernel_lock, so no other
         * grower can slip in between.
         */
        bool can_grow = (ipc_pool_count_locked(server) < IPC_TASK_POOL_MAX_NUM);
        proc_ipc_server_unlock(server);
        if(!can_grow)
            return NULL; //pool at max and all busy: caller blocks the client
        worker = proc_ipc_pool_spawn(serv_proc);
        if(worker == NULL)
            return NULL; //resources exhausted: caller blocks the client
        proc_ipc_server_lock(server);
        ipc_pool_register_locked(server, worker);
        ipc_pool_bind_locked(server, worker, ipc);
    }
    proc_ipc_server_unlock(server);

    proc_wakeup(worker); //BLOCK -> READY; enters the rewritten ctx
    return worker;
}

/*
 * Idle-shrink sweep (called from the 1Hz renew_kernel_sec tick): when the
 * pool holds more live members than IPC_TASK_POOL_MIN_NUM, mark ONE member
 * that sat parked idle for at least IPC_TASK_SELF_QUIT_TIMEOUT seconds for
 * self-termination and wake it - the worker notices the quit mark right
 * after waking from its park and exits, releasing its proc slot and thread
 * stack. The MIN_NUM base members are never targeted because the sweep
 * stops once only MIN_NUM members are left. Runs in timer interrupt
 * context; all pool state is touched under the same server lock that
 * assignment and park use, so a worker is never quit-marked while a
 * request is being bound to it.
 */
void proc_ipc_pool_shrink(proc_t* serv_proc) {
    if(serv_proc == NULL || serv_proc->space == NULL)
        return;
    ipc_server_t* server = &serv_proc->space->ipc_server;
    if(!server->multi_task || server->entry == 0)
        return;

    proc_t* target = NULL;
    proc_ipc_server_lock(server);
    uint32_t live = ipc_pool_count_locked(server);
    if(live > IPC_TASK_POOL_MIN_NUM) {
        for(uint32_t i = 0; i < IPC_TASK_POOL_MAX_NUM; i++) {
            proc_t* w = ipc_pool_slot_worker(server, i);
            if(w == NULL || server->pool[i].quit ||
                    w->info.state != BLOCK || w->ipc_task != NULL)
                continue;
            if(server->pool[i].idle_sec == 0 ||
                    (_kernel_info.uptime_sec - server->pool[i].idle_sec) < IPC_TASK_SELF_QUIT_TIMEOUT)
                continue;
            server->pool[i].quit = 1;
            target = w;
            break; //one per second; the next tick takes the next member
        }
    }
    proc_ipc_server_unlock(server);

    if(target != NULL)
        proc_wakeup(target); //the woken worker sees the quit mark and exits
}

/*
 * Block the client until a pool worker parks (becomes idle). Same shape
 * as the old stack-slot wait: the re-check happens under the same server
 * lock the park path flips its BLOCK state with, so a worker parking
 * right before we enqueue still wakes us (the park issues
 * proc_ipc_wakeup() after the transition). Returns 1 when a worker is
 * already idle again (caller retries at once), 0 after a block/wake
 * cycle.
 */
static int32_t proc_ipc_wait_pool(context_t* ctx, proc_t* serv_proc, proc_t* proc) {
    if(serv_proc == NULL || proc == NULL || serv_proc->space == NULL)
        return -1;

    ipc_pool_replenish(serv_proc); //a missing member may be spawnable now

    if(proc->ipc_waiting_on != NULL && proc->ipc_waiting_on != serv_proc)
        proc_ipc_cancel_wait(proc);

    ipc_queue_item_t* item = &proc->ipc_wait_item;
    item->owner = proc;
    item->pid = proc->info.pid;
    item->uuid = proc->info.uuid;
    ipc_server_t* server = &serv_proc->space->ipc_server;
    proc_ipc_server_lock(server);
    if(!server->disabled && ipc_pool_has_idle(server)) {
        proc_ipc_server_unlock(server);
        return 1;
    }
    if(!item->queued) {
        ipc_waitq_link_tail(server, item);
        ipc_wait_item_set_server(item, serv_proc);
    }
    proc_ipc_server_unlock(server);
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

/* ======================================================================
 * Syscall-level ipc entry points (SYS_IPC_* implementations). svc.c only
 * dispatches into these.
 * ====================================================================== */

/* SYS_IPC_CALL */
void proc_ipc_call(context_t* ctx, int32_t serv_pid, int32_t call_id, proto_t* arg) {
    ctx->gpr[0] = 0;

    proc_t* client_proc = get_current_proc();
    serv_pid = get_proc_pid(serv_pid);
    proc_t* serv_proc = proc_get(serv_pid);

    if(client_proc->info.pid == serv_pid) { //can't do self ipc
        printf("ipc can't call self service (client: %d, server: %d, call: 0x%x\n", client_proc->info.pid, serv_pid, call_id);
        ctx->gpr[0] = IPC_ERROR_SELF;
        return;
    }

    if(serv_proc == NULL ||
            serv_proc->space->ipc_server.entry == 0) {//no ipc service setup
        ctx->gpr[0] = IPC_ERROR_NO_READY;
        return;
    }

    if(serv_proc->space->ipc_server.disabled) {
        ctx->gpr[0] = IPC_ERROR_RETRY; // blocked if server disabled, should retry
        proc_ipc_wait(ctx, serv_proc, client_proc);
        return;
    }

    if(serv_proc->space->ipc_server.multi_task) {
        /*
         * multi_task server: the server's main context is never hijacked.
         * Requests are served by a persistent pool of worker threads
         * inside the server proc (IPC_TASK_POOL_MIN_NUM created up front,
         * grown on demand up to IPC_TASK_POOL_MAX_NUM, members parked
         * between requests), each running the ipc entry, so requests are
         * served concurrently. No interrupt-state gate here either: a
         * worker thread is an independent context and can safely run
         * while the server's main context handles an interrupt.
         *
         * When the pool is at its maximum and no worker is idle (or no
         * task slot is free) the client is kept blocked INSIDE the
         * kernel until a worker parks; returning RETRY unblocked would
         * make userspace ipc_call busy-loop.
         */
        uint32_t serv_uuid = serv_proc->info.uuid;
        while(true) {
            ipc_pool_replenish(serv_proc);
            ipc_task_t* ipc = proc_ipc_req(serv_proc, client_proc, call_id, arg);
            if(ipc == NULL) { //all task slots busy
                ctx->gpr[0] = IPC_ERROR_RETRY;
                proc_ipc_wait(ctx, serv_proc, client_proc);
            }
            else {
                proc_cur_ipc_res(client_proc)->state = IPC_BUSY;
                ctx->gpr[0] = ipc->uid;
                if(ipc_pool_assign(serv_proc, ipc) != NULL)
                    return;
                //no idle pool worker: free the task slot, block and retry
                proc_ipc_close(serv_proc, ipc);
                proc_ipc_wait_pool(ctx, serv_proc, client_proc);
            }

            /*
             * Woken for a retry: the server may have exited meanwhile.
             * Re-resolve it by pid+uuid before touching it again.
             */
            serv_proc = proc_get(serv_pid);
            if(serv_proc == NULL ||
                    serv_proc->info.uuid != serv_uuid ||
                    serv_proc->space == NULL ||
                    !serv_proc->space->ipc_server.multi_task) {
                ctx->gpr[0] = IPC_ERROR_NO_READY;
                return;
            }
            if(serv_proc->space->ipc_server.disabled) {
                ctx->gpr[0] = IPC_ERROR_RETRY;
                proc_ipc_wait(ctx, serv_proc, client_proc);
            }
        }
    }

    if(serv_proc->space->interrupt.state != INTR_STATE_IDLE) {
        ctx->gpr[0] = IPC_ERROR_RETRY; // blocked if proc is on interrupt task, should retry
        proc_interrupt_wait(ctx, serv_proc, client_proc);
        return;
    }

    ipc_task_t* ipc = proc_ipc_req(serv_proc, client_proc, call_id, arg);
    if(ipc == NULL) {
        ctx->gpr[0] = IPC_ERROR_RETRY;
        proc_ipc_wait(ctx, serv_proc, client_proc);
        return;
    }

    proc_cur_ipc_res(client_proc)->state = IPC_BUSY;
    ctx->gpr[0] = ipc->uid;
    if(ipc == proc_ipc_get_task(serv_proc))
        proc_ipc_do_task(ctx, serv_proc, client_proc->info.core);
}

/* SYS_IPC_GET_RETURN */
void proc_ipc_get_return(context_t* ctx, int32_t serv_pid, uint32_t uid, proto_t* data) {
    ctx->gpr[0] = 0;
    proc_t* client_proc = get_current_proc();
    if(uid == 0 || client_proc == NULL) {
        ctx->gpr[0] = -2;
        return;
    }
    serv_pid = get_proc_pid(serv_pid);

    ipc_res_t* res = proc_cur_ipc_res(client_proc);
    if(res->state != IPC_RETURN) { //block retry for serv return
        proc_t* serv_proc = proc_get(serv_pid);
        ipc_task_t* ipc;
        /*
         * multi_task servers complete requests out of order in worker
         * threads, so the queue head says nothing about THIS call - look
         * the task up by its uid instead.
         */
        if(serv_proc != NULL && serv_proc->space != NULL &&
                serv_proc->space->ipc_server.multi_task)
            ipc = proc_ipc_find_task(serv_proc, uid);
        else
            ipc = proc_ipc_get_task(serv_proc);
        if(ipc == NULL) {
            ctx->gpr[0] = -2;
            return;
        }

        if((ipc->call_id & IPC_NON_RETURN) == 0 || ipc->uid != uid) {
            ctx->gpr[0] = -1;
            proc_block(ctx, client_proc);
            return;
        }
        return;
    }

    if(res->uid != uid) {
        ctx->gpr[0] = -2;
        return;
    }

    if(data != NULL && data->data != NULL) {
        if(data->total_size >= res->data.size)
            proto_copy(data, res->data.data, res->data.size);
        else {
            ctx->gpr[0] = res->data.size; //return ret_arg size if input pkg not big enought
            return;
        }
    }

    res->uid = 0;
    res->state = IPC_IDLE;
    proto_clear(&res->data);
}

/* SYS_IPC_GET_ARG */
int32_t proc_ipc_get_arg(uint32_t uid, int32_t* ipc_info, proto_t* arg) {
    proc_t* cproc = get_current_proc();
    if(cproc == NULL || cproc->space == NULL ||
            cproc->space->ipc_server.entry == 0)
        return -1;

    ipc_task_t* ipc = proc_ipc_serving_task(cproc, uid);
    if(ipc == NULL)
        return -1;

    ipc_info[0] = ipc->client_pid;
    ipc_info[1] = ipc->call_id;

    if(arg != NULL) {
        if(arg->total_size >= ipc->arg_ret.size && ipc->arg_ret.data != NULL)
            proto_copy(arg, ipc->arg_ret.data, ipc->arg_ret.size);
        else
            return ipc->arg_ret.size;
    }
    return 0;
}

/* SYS_IPC_SET_RETURN */
void proc_ipc_set_return(uint32_t uid, proto_t* data) {
    proc_t* cproc = get_current_proc();
    if(cproc == NULL || cproc->space == NULL ||
            cproc->space->ipc_server.entry == 0)
        return;

    ipc_task_t* ipc = proc_ipc_serving_task(cproc, uid);
    if(ipc == NULL ||
            (ipc->call_id & IPC_NON_RETURN) != 0) {
        return;
    }

    proc_t* client_proc = proc_ipc_get_client(ipc);
    if(client_proc != NULL) {
        ipc_res_t* res = proc_ipc_client_res(client_proc, ipc);
        res->state = IPC_RETURN;
        res->uid = uid;
        if(data != NULL) {
            proto_copy(&res->data, data->data, data->size);
        }
    }
}

/*
 * multi_task SYS_IPC_END: runs in the worker thread that served the
 * request. The return data was already delivered to the client by
 * SYS_IPC_SET_RETURN; here the delivery is finished: the task slot is
 * released and the client plus any queued callers are woken. Pool workers
 * then PARK (keep their proc slot and stack, wait for the next request)
 * instead of exiting - spawning/burying a thread per request was the
 * cost this pool removes. The server's main context and saved_state are
 * never involved.
 */
static void ipc_end_multi(context_t* ctx, proc_t* worker, proc_t* serv_proc) {
    ipc_server_t* server = &serv_proc->space->ipc_server;
    ipc_task_t* ipc = worker->ipc_task;
    worker->ipc_task = NULL;

    bool pool_member;
    proc_ipc_server_lock(server);
    pool_member = ipc_pool_is_member(server, worker);
    proc_ipc_server_unlock(server);

    if(ipc != NULL) {
        bool wake_return_client = false;
        proc_t* client_proc = proc_ipc_finish_task(serv_proc, ipc, &wake_return_client);

        if(pool_member) {
            proc_ipc_pool_park(ctx, worker, serv_proc,
                    wake_return_client ? client_proc : NULL);
            return; //woken later with a new request context (or terminated with the server)
        }

        /*
         * Non-pool worker: release its thread stack right away instead of
         * leaving it pinned on the zombie until the funeral sweep (up to
         * 1s later). proc_free_user_stack() in the funeral guards on
         * thread_stack_base != 0, so no double free. Runs in the worker's
         * own context, so the active translation table is exactly the
         * space this stack belongs to.
         */
        if(worker->thread_stack_base != 0) {
            thread_stack_free(worker, worker->thread_stack_base);
            worker->thread_stack_base = 0;
        }

        proc_ipc_wakeup(serv_proc); //let blocked callers into the freed slot
        if(wake_return_client && client_proc != NULL)
            proc_wakeup(client_proc);
        proc_exit(ctx, worker, 0);
        return;
    }

    /*
     * No task attached to this context. For a worker thread this happens
     * when the watchdog already aborted (and detached) its timed-out
     * request. A pool member just parks again for the next request; a
     * leftover non-pool thread terminates. Userspace only ever calls
     * SYS_IPC_END from inside the ipc handler, so no unrelated thread
     * reaches this path; the main proc context still gets a no-op.
     */
    if(pool_member) {
        proc_ipc_pool_park(ctx, worker, serv_proc, NULL);
        return;
    }
    if(worker->info.type == TASK_TYPE_THREAD) {
        proc_exit(ctx, worker, 0);
    }
}

/* SYS_IPC_END */
void proc_ipc_end(context_t* ctx) {
    proc_t* cproc = get_current_proc();
    proc_t* serv_proc = proc_get_proc(cproc);
    if(serv_proc == NULL || serv_proc->space == NULL ||
            serv_proc->space->ipc_server.entry == 0)
        return;

    if(serv_proc->space->ipc_server.multi_task) {
        ipc_end_multi(ctx, cproc, serv_proc);
        return;
    }

    ipc_task_t* ipc = proc_ipc_get_task(serv_proc);
    if(ipc == NULL)
        return;

    proc_t* client_proc = proc_ipc_get_client(ipc);
    bool wake_return_client = ((ipc->call_id & IPC_NON_RETURN) == 0);
    bool throughput_mode = ((serv_proc->space->ipc_server.flags & IPC_NON_BLOCK) != 0);

    serv_proc->space->ipc_server.restore_pending = 0;
    proc_restore_state(ctx, serv_proc, &serv_proc->space->ipc_server.saved_state, &serv_proc->space->ipc_server.saved_ipc_res);

    // Fix: If the server was BLOCK/SLEEPING when the IPC arrived,
    // make it READY so it can be re-scheduled to continue its main loop.
    // Without this, the server gets stranded in BLOCK with no wakeup source.
    if(serv_proc->info.state == BLOCK || serv_proc->info.state == SLEEPING) {
        serv_proc->info.state = READY;
    }

    if(serv_proc->info.state == READY || serv_proc->info.state == RUNNING) {
        proc_ready(serv_proc);
    }
    proc_ipc_close(serv_proc, ipc);
    proc_ipc_wakeup(serv_proc);
    ipc_task_t* next_ipc = proc_ipc_get_task(serv_proc);
    if(wake_return_client &&
            client_proc != NULL &&
            client_proc->info.state != UNUSED &&
            client_proc->info.state != ZOMBIE) {
        /*
         * Do not wake the caller from sys_ipc_set_return() before the server
         * fully closes the current ctask. Otherwise the caller can immediately
         * issue the next ipc_call(), hit IPC_ERROR_RETRY against the still-busy
         * server slot, and spin in short RET->END races that amplify VFS
         * metadata-heavy workloads into multi-second boot tails.
         */
        proc_wakeup(client_proc);
        if(!throughput_mode && next_ipc == NULL) {
            proc_switch_multi_core(ctx, client_proc, serv_proc->info.core);
            return;
        }
    }

    if(next_ipc != NULL) {
        proc_ipc_do_task(ctx, serv_proc, serv_proc->info.core);
        return;
    }
    schedule(ctx);
}

/* SYS_IPC_DISABLE */
int32_t proc_ipc_disable(void) {
    proc_t* cproc = proc_get_proc(get_current_proc());
    if(proc_ipc_task_count(cproc) > 0) //still serving requests (any slot/thread)
        return -1;
    cproc->space->ipc_server.disabled = true;
    return 0;
}

/* SYS_IPC_ENABLE */
void proc_ipc_enable(void) {
    proc_t* cproc = proc_get_proc(get_current_proc());
    if(!cproc->space->ipc_server.disabled)
        return;

    cproc->space->ipc_server.disabled = false;
    proc_ipc_wakeup(cproc);
}
