/*
 * proc.c - process lifecycle: fd slot tracking, clone/exit and zombie reaping.
 */
#include "vfsd.h"

queue_t _zombie_tasks;

static bool queue_zombie_task_match(void* data, void* check_data) {
    if(data == NULL || check_data == NULL)
        return false;
    zombie_task_t* task = (zombie_task_t*)data;
    zombie_task_t* check = (zombie_task_t*)check_data;
    return task->pid == check->pid && task->uuid == check->uuid;
}

/* caller must hold _vfs_lock (write) */
void remove_zombie_task(int32_t pid, uint32_t uuid) {
    if(uuid == 0)
        return;

    zombie_task_t check;
    check.pid = pid;
    check.uuid = uuid;

    queue_item_t* it = _zombie_tasks.head;
    while(it != NULL) {
        queue_item_t* next = it->next;
        if(queue_zombie_task_match(it->data, &check)) {
            free(it->data);
            queue_remove(&_zombie_tasks, it);
        }
        it = next;
    }
}

/* caller must hold _vfs_lock (write) */
static void vfs_remove_proc_waiters(int32_t pid, uint32_t uuid) {
    if(pid < 0 || (uint32_t)pid >= _max_proc_table_num || uuid == 0)
        return;

    wait_entry_t* read_waiter = get_wait_entry(pid, false);
    wait_entry_t* write_waiter = get_wait_entry(pid, true);
    if(read_waiter != NULL && read_waiter->uuid == uuid)
        wait_queue_remove_entry(read_waiter);
    if(write_waiter != NULL && write_waiter->uuid == uuid)
        wait_queue_remove_entry(write_waiter);
}

/* caller must hold _vfs_lock (write) */
static void vfs_proc_exit(int32_t cpid) {
    if(cpid < 0 || (uint32_t)cpid >= _max_proc_table_num)
        return;
    if(_proc_fds_table[cpid].uuid == 0) {
        wait_queue_remove_entry(&_proc_fds_table[cpid].read_waiter);
        wait_queue_remove_entry(&_proc_fds_table[cpid].write_waiter);
        memset(&_proc_fds_table[cpid].read_waiter, 0, sizeof(wait_entry_t));
        memset(&_proc_fds_table[cpid].write_waiter, 0, sizeof(wait_entry_t));
        /*
         * A zero uuid does NOT mean "nothing to clean": core forwards
         * KEV_PROC_CREATED/EXIT through a polled kevent queue (~50ms), so a
         * short-lived process (e.g. `ps` in a shell pipeline on fast
         * hardware) can already be dead when VFS_PROC_CLONE finally runs.
         * do_vfs_proc_clone() then still copies the parent's fds and bumps
         * node refs/refs_w (and dups driver-side refs via FS_CMD_DUP), but
         * records uuid=0. Skipping fd cleanup here leaks those references
         * forever: a pipe write end never drops (its reader never sees EOF)
         * and a driver's per-fd ref never returns (netd connection task
         * never released). Reap the slot's fds like any other zombie;
         * clear_zombie() also resets the slot to UNUSED.
         */
        clear_zombie(cpid);
        return;
    }
    /*
     * Generation guard: KEV_PROC_EXIT carries ONLY a pid and travels through
     * core's polled kevent queue (~50ms lag), while threads created with the
     * same recycled pid start running IMMEDIATELY (kfork readies threads
     * without the CREATED handshake used for procs). So by the time a stale
     * EXIT lands here, the slot may already be re-materialized for a LIVE new
     * generation (vfs_track_task_slot/do_vfs_proc_clone reaped the old one at
     * reuse time). Reaping in that case removes the live task's wait-queue
     * entry (it sleeps in the kernel forever) and closes its fds behind its
     * back - unrelated processes wedged by mere pid churn. The kernel zeroes
     * the vsyscall uuid at proc_terminate() BEFORE core can forward the
     * event, so "slot generation still alive" proves this EXIT belongs to an
     * earlier occupant: skip it. A genuine exit always fails this check and
     * reaps exactly as before.
     */
    if(proc_check_uuid(cpid, _proc_fds_table[cpid].uuid) != 0)
        return;
    _proc_fds_table[cpid].state = ZOMBIE;
    /*
     * Process exit cleanup must be deterministic. The deferred zombie queue
     * only exists to postpone clear_zombie(), but clear_zombie() no longer
     * performs synchronous FS_CMD_CLOSE; driver-side closes are already
     * handed off to the worker thread. Reap the slot immediately here so a
     * stale/corrupted _zombie_tasks traversal cannot strand refs and leave
     * xconsole umount permanently deferred.
     *
     * Duplicate EXIT notifications are harmless: after clear_zombie() the
     * slot uuid becomes 0, and the next pass takes the zero-uuid branch and
     * returns without touching live refs.
     */
    clear_zombie(cpid);
}

/* caller must hold _vfs_lock (write) */
void clear_zombie(int32_t cpid) {
    if(cpid < 0)
        return;
    vfs_remove_proc_waiters(cpid, _proc_fds_table[cpid].uuid);
    int32_t owner_pid = get_tracked_owner_pid(cpid);

    int32_t i;
    for(i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
        file_t *f = &_proc_fds_table[cpid].fds[i];
        if(f->node != NULL) {
            file_t closing = *f;
            memset(f, 0, sizeof(file_t));
            uint32_t type = FS_BASE_TYPE(closing.fsinfo.type);
            /*
             * Queue exactly one FS_CMD_CLOSE per fd that owns a driver-side
             * reference (see file_t.driver_ref). Fork sends FS_CMD_DUP per
             * inherited fd, so a process can hold several ref-owning fds of
             * the same node (e.g. socket stdin + original fd + backup fd in
             * a telnet shell's command children); deduplicating per node
             * under-closes and leaks the driver task. dup2-created fds own
             * no ref and must not be closed here.
             */
            if(type != FS_TYPE_FILE &&
                    type != FS_TYPE_DIR &&
                    type != FS_TYPE_LINK &&
                    closing.driver_ref &&
                    closing.fsinfo.mount_pid > 0 &&
                    closing.fsinfo.node != 0) {
                driver_close_task_t* task =
                    (driver_close_task_t*)malloc(sizeof(driver_close_task_t));
                if(task != NULL) {
                    task->pid = cpid;
                    task->owner_pid = owner_pid;
                    task->fd = i;
                    task->file = closing;
                    enqueue_driver_close_task(task);
                }
            }
            proc_file_close(cpid, i, &closing);
            continue;
        }
        memset(f, 0, sizeof(file_t));
    }

    _proc_fds_table[cpid].state = UNUSED;
    _proc_fds_table[cpid].uuid = 0;
    _proc_fds_table[cpid].owner_pid = 0;
}

/* ipc_serv "handled" callback: runs in a worker thread after each request */
void clear_pending_zombies(void* p) {
    (void)p;
    while(true) {
        pthread_rwlock_wrlock(&_vfs_lock);
        zombie_task_t* task = (zombie_task_t*)queue_pop(&_zombie_tasks);
        if(task == NULL) {
            pthread_rwlock_unlock(&_vfs_lock);
            return;
        }
        if(task->pid >= 0 &&
                (uint32_t)task->pid < _max_proc_table_num &&
                _proc_fds_table[task->pid].state == ZOMBIE &&
                _proc_fds_table[task->pid].uuid == task->uuid) {
            clear_zombie(task->pid);
            pthread_rwlock_unlock(&_vfs_lock);
            free(task);
            return;
        }
        pthread_rwlock_unlock(&_vfs_lock);
        free(task);
    }
}

/* caller must hold _vfs_lock (write) */
static void vfs_reset_task_waiters(int32_t pid) {
    if(pid < 0 || (uint32_t)pid >= _max_proc_table_num)
        return;
    wait_queue_remove_entry(&_proc_fds_table[pid].read_waiter);
    wait_queue_remove_entry(&_proc_fds_table[pid].write_waiter);
    memset(&_proc_fds_table[pid].read_waiter, 0, sizeof(wait_entry_t));
    memset(&_proc_fds_table[pid].write_waiter, 0, sizeof(wait_entry_t));
}

/* caller must hold _vfs_lock (write) */
void vfs_track_task_slot(int32_t pid) {
    uint32_t uuid;

    if(pid < 0 || (uint32_t)pid >= _max_proc_table_num)
        return;
    uuid = proc_get_uuid(pid);
    if(uuid == 0)
        return;

    if((_proc_fds_table[pid].state == RUNNING ||
            _proc_fds_table[pid].state == ZOMBIE) &&
            _proc_fds_table[pid].uuid != 0 &&
            _proc_fds_table[pid].uuid != uuid) {
        uint32_t old_uuid = _proc_fds_table[pid].uuid;
        remove_zombie_task(pid, old_uuid);
        /*
         * exec() reloads a process image in-place and the kernel assigns a fresh
         * uuid to the same process pid. The process-owned fd table must survive
         * that transition; only old poll/block waiters tied to the previous image
         * should be dropped. Reusing a different pid slot (threads or real pid
         * reuse after exit) still goes through the full zombie cleanup path.
         */
        if(_proc_fds_table[pid].state == RUNNING && vfs_fd_owner_pid(pid) == pid) {
            vfs_remove_proc_waiters(pid, old_uuid);
            vfs_reset_task_waiters(pid);
            _proc_fds_table[pid].uuid = uuid;
            return;
        }
        _proc_fds_table[pid].state = ZOMBIE;
        clear_zombie(pid);
    }

    _proc_fds_table[pid].state = RUNNING;
    _proc_fds_table[pid].owner_pid = vfs_fd_owner_pid(pid);
    _proc_fds_table[pid].uuid = uuid;
}

void do_vfs_proc_exit(int32_t pid, proto_t* in) {
    (void)pid;
    int cpid = proto_read_int(in);

    if(cpid < 0 || (uint32_t)cpid >= _max_proc_table_num)
        return;
    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_proc_exit(cpid);
    pthread_rwlock_unlock(&_vfs_lock);
}
