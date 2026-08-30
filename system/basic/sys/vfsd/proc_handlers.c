/*
 * proc_handlers.c - process fork/clone and block/wakeup/poll IPC handlers.
 */
#include "vfsd.h"

void do_vfs_proc_clone(int32_t pid, proto_t* in) {
    (void)pid;
    int fpid = proto_read_int(in);
    int cpid = proto_read_int(in);
    if(fpid < 0 || (uint32_t)fpid >= _max_proc_table_num ||
            cpid < 0 || (uint32_t)cpid >= _max_proc_table_num)
        return;

    bool child_dead = false;
    bool dup_wait_needed = false;
    int32_t dup_wait_mount = 0;
    /* fds whose async dup job could not be queued: synchronous fallback below */
    int32_t fb_fds[MAX_OPEN_FILE_PER_PROC];
    int32_t fb_num = 0;
    clone_dup_ctx_t* dup_ctx = clone_dup_ctx_create();

    pthread_rwlock_wrlock(&_vfs_lock);
    if(_proc_fds_table[cpid].state == RUNNING ||
            _proc_fds_table[cpid].state == ZOMBIE) {
        uint32_t old_uuid = _proc_fds_table[cpid].uuid;
        /*
         * The child slot is about to be reused immediately. We cannot leave the
         * previous occupant queued for later async reaping, otherwise its stale
         * fds/refs survive into the reused slot and the delayed zombie task may
         * race with the new child. Reap the old slot synchronously here and drop
         * any queued zombie task for the same generation.
         */
        _proc_fds_table[cpid].state = ZOMBIE;
        remove_zombie_task(cpid, old_uuid);
        clear_zombie(cpid);
    }

    _proc_fds_table[cpid].state = RUNNING;
    _proc_fds_table[cpid].owner_pid = vfs_fd_owner_pid(cpid);
    _proc_fds_table[cpid].uuid = proc_get_uuid(cpid);
    child_dead = (_proc_fds_table[cpid].uuid == 0);
    int32_t i;
    for(i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
        file_t *f = &_proc_fds_table[fpid].fds[i];
        vfs_node_t* node = f->node;
        if(node != NULL) {
            file_t* file = &_proc_fds_table[cpid].fds[i];
            uint32_t type = FS_BASE_TYPE(f->fsinfo.type);
            bool needs_driver_dup = (type != FS_TYPE_FILE &&
                    type != FS_TYPE_DIR &&
                    type != FS_TYPE_LINK &&
                    f->fsinfo.mount_pid > 0);
            memcpy(file, f, sizeof(file_t));
            node->refs++;
            if((f->flags & (O_WRONLY | O_RDWR)) != 0)
                node->refs_w++;
            /*
             * Fork only needs a driver-side ref when the inherited fd is
             * backed by a real mount driver. Local VFS-only nodes such as
             * pipes keep all runtime state inside vfsd itself, so they must
             * not be reported as failed "driver dup" work.
             */
            file->driver_ref = needs_driver_dup ? 1 : 0;
            if(needs_driver_dup) {
                bool queued = false;

                /*
                 * Pipes never take the async dup queue: piped derives
                 * reader/writer-closed from its descriptor refcount, and an
                 * inherited pipe end must be counted BEFORE fork returns.
                 * A queued +1 races the fast direct FS_CMD_CLOSE that libc
                 * sends from any process (shell closing its pipe fds, a
                 * quick child exiting); the transient zero marks the pipe
                 * closed while descriptors are still alive and writers get
                 * EPIPE / readers EOF mid-pipeline. The sync fallback below
                 * completes before the parent leaves VFS_PROC_CLONE.
                 */
                if(dup_ctx != NULL &&
                        !FS_IS_TYPE(f->fsinfo.type, FS_TYPE_PIPE)) {
                    queued = queue_driver_dup_job(dup_ctx, file->fsinfo.mount_pid,
                            fpid, i, cpid, i, file);
                }
                if(queued) {
                    dup_wait_needed = true;
                    if(dup_wait_mount <= 0)
                        dup_wait_mount = file->fsinfo.mount_pid;
                }
                else if(fb_num < MAX_OPEN_FILE_PER_PROC) {
                    fb_fds[fb_num++] = i;
                }
            }
        }
    }
    pthread_rwlock_unlock(&_vfs_lock);

    /*
     * Synchronous fallback FS_CMD_DUP round-trips must happen OUTSIDE
     * _vfs_lock: the target driver may call back into vfsd, and that
     * callback would deadlock on the lock we still hold. The child's fd
     * slot is re-validated under a short read lock before each round trip.
     */
    for(int32_t k = 0; k < fb_num; k++) {
        int32_t fd_i = fb_fds[k];
        file_t snapshot;
        int32_t mount_pid;

        pthread_rwlock_rdlock(&_vfs_lock);
        file_t* file = vfs_get_file(cpid, fd_i);
        if(file != NULL && file->node != NULL) {
            snapshot = *file;
            mount_pid = snapshot.fsinfo.mount_pid;
        } else {
            snapshot.node = NULL;
            mount_pid = 0;
        }
        pthread_rwlock_unlock(&_vfs_lock);

        if(snapshot.node == NULL)
            continue;
        if(vfs_driver_dup_now(mount_pid, fpid, fd_i, cpid, fd_i, &snapshot) != 0) {
            klog("vfsd: driver dup failed mount=%d from=%d:%d dup=%d:%d node=%u\n",
                    mount_pid, fpid, fd_i, cpid, fd_i, snapshot.fsinfo.node);
            /*
             * The +1 never reached the driver: drop the driver_ref mark so
             * clear_zombie() does not ship an unmatched FS_CMD_CLOSE at exit
             * (that would drive piped's refcount below the true descriptor
             * count and close a pipe some other holder still uses).
             */
            pthread_rwlock_wrlock(&_vfs_lock);
            file_t* recheck = vfs_get_file(cpid, fd_i);
            if(recheck != NULL && recheck->node == snapshot.node)
                recheck->driver_ref = 0;
            pthread_rwlock_unlock(&_vfs_lock);
        }
    }

    /* sleeps/waits on the driver dup completion: never under _vfs_lock */
    if(dup_ctx != NULL) {
        if(dup_wait_needed)
            clone_dup_ctx_wait(dup_ctx, dup_wait_mount);
        clone_dup_ctx_unref(dup_ctx);
    }
    /*
     * fork() notifies vfsd synchronously via VFS_PROC_CLONE, but process
     * create/exit lifecycle still reaches us through the polled core event
     * queue. A short-lived child can therefore already be gone when clone
     * runs, so the fd table above is materialized with uuid==0. If the
     * earlier EXIT event has already been observed, no later callback will
     * revisit this slot, leaving pipe refs/refs_w and driver refs leaked.
     * Reap the just-cloned dead child immediately.
     */
    if(child_dead) {
        pthread_rwlock_wrlock(&_vfs_lock);
        _proc_fds_table[cpid].state = ZOMBIE;
        clear_zombie(cpid);
        pthread_rwlock_unlock(&_vfs_lock);
    }
}

void do_vfs_block(int32_t pid, proto_t* in) {
    int node_id = proto_read_int(in);
    int events = proto_read_int(in);

    if(node_id == 0)
        return;

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    /*
     * ALWAYS enqueue, even if the requested event bit is currently set.
     * Skipping registration here ("event already pending, client will see
     * it") races with third parties clearing node->events between this
     * handler returning and the client's re-check in vfs_block():
     * device drivers and other userspace/device paths may consume sticky
     * bits between the registration IPC and the caller's post-register
     * visibility check. If
     * the bit vanishes in that window the client blocks while
     * registered on NO queue and nobody ever wakes it - the "unrelated
     * process hangs forever" failure mode. Registering unconditionally is
     * safe: if the client's re-check still sees the event it calls
     * VFS_UNBLOCK, which removes the entry; a leftover entry only costs one
     * spurious wakeup, and vfs_block()'s loop re-checks poll state anyway.
     */
    vfs_track_task_slot(pid);
    uint32_t uuid = proc_get_uuid(pid);
    if(uuid == 0) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    bool rd = (events & VFS_EVT_RD) != 0;
    bool wr = (events & VFS_EVT_WR) != 0;

    /*
     * A poll()/select() waiter can watch a single fd for both read and
     * write readiness. Read edges only wake the read queue and write edges
     * only wake the write queue, so a dual-interest waiter must live on
     * BOTH queues; registering on one alone strands the other edge and the
     * poll blocks forever. A waiter interested only in close/err/nval (no
     * RD/WR) is parked on the read queue since those events wake both.
     */
    if(rd || (!rd && !wr))
        enqueue_waiter(&node->read_wait_queue, pid, uuid, false, (uint32_t)node_id);
    if(wr)
        enqueue_waiter(&node->write_wait_queue, pid, uuid, true, (uint32_t)node_id);
    pthread_rwlock_unlock(&_vfs_lock);
}

void do_vfs_wakeup(int32_t pid, proto_t* in) {
    (void)pid;
    int node_id = proto_read_int(in);
    int events = proto_read_int(in);
    if(node_id == 0)
        return;

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    do_node_wakeup(node, events);
    pthread_rwlock_unlock(&_vfs_lock);
}

/*
 * Remove the caller's waiter from a single node's queues. A process must be
 * registered on a node ONLY while it is actually blocked waiting on it;
 * otherwise a stale entry lets an unrelated node event (e.g. tty keyboard RD)
 * spuriously wake a process now blocked elsewhere (e.g. a shell write), because
 * the kernel proc_wakeup() is unconditional and not tied to any node.
 */
void do_vfs_unblock(int32_t pid, proto_t* in) {
    int node_id = proto_read_int(in);
    if(node_id == 0)
        return;

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    uint32_t uuid = proc_get_uuid(pid);
    wait_entry_t* read_waiter;
    wait_entry_t* write_waiter;
    if(uuid == 0) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }
    read_waiter = get_wait_entry(pid, false);
    write_waiter = get_wait_entry(pid, true);
    if(read_waiter != NULL && read_waiter->uuid == uuid &&
            read_waiter->queue == &node->read_wait_queue) {
        wait_queue_remove_entry(read_waiter);
    }
    if(write_waiter != NULL && write_waiter->uuid == uuid &&
            write_waiter->queue == &node->write_wait_queue) {
        wait_queue_remove_entry(write_waiter);
    }
    pthread_rwlock_unlock(&_vfs_lock);
}

void do_vfs_get_poll_events(int32_t pid, proto_t* in, proto_t* out) {
    PF->addi(out, 0);
    if(pid < 0 || (uint32_t)pid >= _max_proc_table_num) {
        return;
    }
    uint32_t node_id = (uint32_t)proto_read_int(in);

    uint32_t events = 0;
    bool have = false;
    pthread_rwlock_rdlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node != NULL) {
        events = node->events;
        have = true;
    }
    pthread_rwlock_unlock(&_vfs_lock);
    if(have)
        PF->clear(out)->addi(out, events);
}

void do_vfs_clear_poll_events(int32_t pid, proto_t* in, proto_t* out) {
    PF->addi(out, -1);
    if(pid < 0 || (uint32_t)pid >= _max_proc_table_num) {
        return;
    }
    uint32_t node_id = (uint32_t)proto_read_int(in);
    uint32_t events = (uint32_t)proto_read_int(in);

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }
    node->events &= ~events;
    pthread_rwlock_unlock(&_vfs_lock);
    PF->addi(out, 0);
}
