/*
 * pipe.c - pipe nodes: open/read/write IPC handlers, shm pipe lifecycle,
 * poll-event sync and the fd-close ref accounting (proc_file_close).
 */
#include "vfsd.h"

/*
 * Retire any shm-pipe block registration owned by 'pid'.
 *
 * The stamp is normally cleared by the stamper itself right after
 * proc_block_by() returns (see read_pipe()/write_pipe() in libc). A task that
 * is killed while blocked never gets there, so its stamp would survive in
 * shared memory and later make some peer call proc_wakeup_by() on a dead pid.
 * SYS_WAKEUP performs no uuid check, so once that pid is recycled the wake
 * hits an unrelated live process with this pipe's node token.
 */
static void pipe_retire_pid_stamps(vfs_node_t* node, int32_t pid) {
    int32_t expect;

    if(node == NULL || node->shm_ring == NULL || pid <= 0)
        return;

    expect = pid;
    __atomic_compare_exchange_n(&node->shm_ring->reader_pid, &expect, 0,
            false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
    expect = pid;
    __atomic_compare_exchange_n(&node->shm_ring->writer_pid, &expect, 0,
            false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
}

/* caller must hold _vfs_lock (write) */
void sync_pipe_poll_events(vfs_node_t* node) {
    if(node == NULL || !FS_IS_TYPE(node->fsinfo.type, FS_TYPE_PIPE))
        return;

    uint32_t events = node->events & ~(VFS_EVT_RD | VFS_EVT_WR);

    if(node->shm_ring != NULL) {
        /* shared-memory pipe: check ring buffer directly */
        if(shm_pipe_readable(node->shm_ring) > 0)
            events |= VFS_EVT_RD;
        if(shm_pipe_writable(node->shm_ring) > 0)
            events |= VFS_EVT_WR;
    } else {
        buffer_t* buffer = (buffer_t*)node->data_ptr;
        if(buffer != NULL) {
            int32_t rest = buffer->size - buffer->offset;
            if(rest > 0)
                events |= VFS_EVT_RD;
            if(rest < BUFFER_SIZE)
                events |= VFS_EVT_WR;
        }
    }
    node->events = events;
}

/* caller must hold _vfs_lock (write) */
void proc_file_close(int pid, int fd, file_t* file) {
    (void)fd;
    if(file == NULL || file->node == NULL)
        return;
    vfs_node_t* node = file->node;
    if(node == NULL)
        return;

    pipe_retire_pid_stamps(node, pid);

    if(node->refs > 0)
        node->refs--;
    if((file->flags & (O_WRONLY|O_RDWR)) != 0 && node->refs_w > 0)
        node->refs_w--;
    bool del_node = false;
    if(FS_IS_TYPE(node->fsinfo.type, FS_TYPE_PIPE)) {
        uint32_t read_refs = (node->refs >= node->refs_w) ? (node->refs - node->refs_w) : 0;
        int32_t unread = 0;

        if(node->shm_ring != NULL) {
            unread = shm_pipe_readable(node->shm_ring);
        } else {
            buffer_t* buffer = (buffer_t*)node->data_ptr;
            if(buffer != NULL) {
                unread = buffer->size - buffer->offset;
                if(unread < 0)
                    unread = 0;
            }
        }

        bool expose_close = (read_refs == 0) || (node->refs_w == 0);
        if(expose_close) {
            node->events |= VFS_EVT_CLOSE;
            /* Set close flags in shared ring so userspace readers/writers
             * can detect close without IPC to vfsd. */
            if(node->shm_ring != NULL) {
                if(node->refs_w == 0) {
                    __atomic_store_n(&node->shm_ring->writer_closed, 1, __ATOMIC_RELEASE);
                    /* Wake shm-path reader directly. The stamp is a ONE-SHOT
                     * block registration (see read_pipe() in libc): consume it
                     * with exchange so it can never fire a second, stale wake
                     * at a recycled pid. */
                    int32_t rpid = __atomic_exchange_n(&node->shm_ring->reader_pid, 0,
                            __ATOMIC_ACQUIRE);
                    if(rpid > 0)
                        proc_wakeup_by(rpid, node->node_id);
                }
                if(read_refs == 0) {
                    __atomic_store_n(&node->shm_ring->reader_closed, 1, __ATOMIC_RELEASE);
                    /* Wake shm-path writer directly (one-shot, see above) */
                    int32_t wpid = __atomic_exchange_n(&node->shm_ring->writer_pid, 0,
                            __ATOMIC_ACQUIRE);
                    if(wpid > 0)
                        proc_wakeup_by(wpid, node->node_id);
                }
            }
        }
        else
            node->events &= ~VFS_EVT_CLOSE;
        sync_pipe_poll_events(node);

        if(node->refs <= 0) {
            if(node->fsinfo.name[0] == 0) { //no refs and not fifo pipe
                if(node->shm_ring != NULL) {
                    shmdt(node->shm_ring);
                    node->shm_ring = NULL;
                }
                buffer_t* buffer = (buffer_t*)node->data_ptr;
                if(buffer != NULL)
                    free(buffer);
                node->data_ptr = NULL;
                del_node = true;
                file->node = 0;
            }
            else {
                node->fsinfo.state = 0;
            }
        }
        if(expose_close)
            do_node_wakeup(node, VFS_EVT_CLOSE);
    }
    else if(FS_IS_ANONYMOUS(node->fsinfo.type)) {
        if(node->refs <= 0) {
            del_node = true;
            file->node = 0;
            do_node_wakeup(node, VFS_EVT_CLOSE);
        }
    }

    if(del_node)
        vfsd_del_node(node);
    else
        vfs_try_finish_umount(node);
    file->node = NULL;
}

void do_vfs_pipe_open(int32_t pid, proto_t* out) {
    PF->addi(out, -1);

    procinfo_t procinfo;
    if(proc_info(pid, &procinfo) != 0)
        return;

    int32_t fd0 = -1;
    int32_t fd1 = -1;
    int32_t shm_id = 0;
    fsinfo_t out_info;
    bool have_info = false;

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node = vfsd_new_node();
    if(node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    node->fsinfo.type = FS_TYPE_PIPE;
    node->fsinfo.stat.mode = 0666;
    node->fsinfo.stat.uid = procinfo.uid;
    node->fsinfo.stat.gid = procinfo.gid;

    /*
     * Allocate a shared-memory ring buffer so reader/writer can transfer
     * data directly without IPC to vfsd. Falls back to the old buffer_t
     * path if shm allocation fails.
     */
    shm_id = shmget(IPC_PRIVATE, SHM_PIPE_PAGE_SIZE, IPC_CREAT | 0666);
    if(shm_id > 0) {
        shm_pipe_t* ring = (shm_pipe_t*)shmat(shm_id, NULL, 0);
        if(ring != (void*)-1) {
            uint32_t nid = vfs_get_node_id(node);
            shm_pipe_init(ring, nid, shm_id);
            node->shm_ring = ring;
            node->data_ptr = NULL;
            node->fsinfo.data = (uint32_t)shm_id;
        } else {
            /* shm map failed, fall back to buffer */
            buffer_t* buf = (buffer_t*)malloc(sizeof(buffer_t));
            memset(buf, 0, sizeof(buffer_t));
            node->data_ptr = buf;
            node->shm_ring = NULL;
            node->fsinfo.data = 0;
            shm_id = 0;
        }
    } else {
        /* shm alloc failed, fall back to buffer */
        buffer_t* buf = (buffer_t*)malloc(sizeof(buffer_t));
        memset(buf, 0, sizeof(buffer_t));
        node->data_ptr = buf;
        node->shm_ring = NULL;
        node->fsinfo.data = 0;
        shm_id = 0;
    }
    sync_pipe_poll_events(node);

    fd0 = vfsd_open(pid, node, O_RDONLY);
    if(fd0 < 0) {
        vfsd_del_node(node);
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    fd1 = vfsd_open(pid, node, O_WRONLY);
    if(fd1 < 0) {
        vfsd_close(pid, fd0);
        vfsd_del_node(node);
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }
    vfs_fill_node_fsinfo(node, &out_info);
    have_info = true;
    pthread_rwlock_unlock(&_vfs_lock);

    PF->clear(out)->addi(out, 0)->
            addi(out, fd0)->
            addi(out, fd1)->
            addi(out, shm_id > 0 ? shm_id : 0);
    if(have_info)
        PF->add(out, &out_info, sizeof(fsinfo_t));
}

void do_vfs_pipe_write(int pid, proto_t* in, proto_t* out) {
    PF->addi(out, -1);

    int32_t fd = proto_read_int(in);
    uint32_t node_id = proto_read_int(in);
    int32_t size = 0;
    void *data = proto_read(in, &size);
    int32_t block = proto_read_int(in);
    if(node_id == 0 || size < 0 || data == NULL)
        return;

    pthread_rwlock_wrlock(&_vfs_lock);

    if(vfs_check_fd(pid, fd) == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    uint32_t read_refs = (node->refs >= node->refs_w) ? (node->refs - node->refs_w) : 0;
    if(read_refs == 0) { // reader side closed
        node->events |= VFS_EVT_CLOSE;
        sync_pipe_poll_events(node);
        do_node_wakeup(node, VFS_EVT_CLOSE);
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    if(node->shm_ring != NULL) {
        /* Write to shared ring buffer (same store used by direct shm path) */
        int32_t n = shm_pipe_write(node->shm_ring, data, size);
        if(n > 0) {
            sync_pipe_poll_events(node);
            do_node_wakeup(node, VFS_EVT_RD);
            /* Also wake the shm-path reader directly (it bypasses wait queue).
             * Consume the one-shot stamp with exchange, exactly like
             * write_pipe() in libc, so it cannot fire again later against a
             * pid that has since been recycled. */
            int32_t rpid = __atomic_exchange_n(&node->shm_ring->reader_pid, 0,
                    __ATOMIC_ACQUIRE);
            if(rpid > 0)
                proc_wakeup_by(rpid, node_id);
            pthread_rwlock_unlock(&_vfs_lock);
            PF->clear(out)->addi(out, n);
            return;
        }
    } else {
        buffer_t* buffer = (buffer_t*)node->data_ptr;
        if(buffer == NULL) {
            sync_pipe_poll_events(node);
            pthread_rwlock_unlock(&_vfs_lock);
            return;
        }
        size = buffer_write(buffer, data, size);
        if(size > 0) {
            node->fsinfo.state |= FS_STATE_CHANGED;
            sync_pipe_poll_events(node);
            do_node_wakeup(node, VFS_EVT_RD);
            pthread_rwlock_unlock(&_vfs_lock);
            PF->clear(out)->addi(out, size);
            return;
        }
    }

    /*
     * Buffer is full. If the caller requested blocking, register the
     * process on the write wait queue atomically.
     *
     * Deliberately NOT stamping shm_ring->writer_pid here. That stamp is a
     * one-shot registration that the STAMPER must retire after its block
     * returns - but this caller is on the IPC fallback path (vfs.c
     * write_pipe()), which does NOT own that shm stamp lifecycle. Let the
     * caller sleep through vfsd's wait queue instead of teaching the fallback
     * path to stamp and retire shared-memory pids it did not register. The old
     * direct-block variant leaked the stamp in shared memory forever, so every
     * later reader of this pipe fired
     * proc_wakeup_by(<dead pid>, <this node>). SYS_WAKEUP does not validate
     * uuid, and pids recycle fast (sshd forks per connection, the shell forks
     * per command), so those wakes landed on unrelated live processes carrying
     * a foreign node token and poisoned their wake latch - the "sshd hangs the
     * X server" bug. The wait queue below is uuid-validated and already
     * covers this waiter: a shm-path reader publishes the full->not-full edge
     * via vfs_wakeup(node, VFS_EVT_WR), which reaches do_node_wakeup().
     */
    if(block) {
        vfs_track_task_slot(pid);
        uint32_t uuid = proc_get_uuid(pid);
        if(uuid != 0)
            enqueue_waiter(&node->write_wait_queue, pid, uuid, true, node_id);
    }

    sync_pipe_poll_events(node);
    pthread_rwlock_unlock(&_vfs_lock);
    PF->clear(out)->addi(out, 0); //buffer full(waiting for read), retry
}

void do_vfs_pipe_read(int pid, proto_t* in, proto_t* out) {
    PF->addi(out, -1);

    int32_t fd = proto_read_int(in);
    uint32_t node_id = proto_read_int(in);
    int32_t size = proto_read_int(in);
    int32_t block = proto_read_int(in);
    if(node_id == 0 || size < 0)
        return;

    pthread_rwlock_wrlock(&_vfs_lock);

    if(vfs_check_fd(pid, fd) == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    if(node->shm_ring != NULL) {
        /* Read from shared ring buffer (same store used by direct shm path) */
        int32_t avail = shm_pipe_readable(node->shm_ring);

        if(avail == 0 && node->refs_w == 0) {
            node->events |= VFS_EVT_CLOSE;
            sync_pipe_poll_events(node);
            do_node_wakeup(node, VFS_EVT_CLOSE);
            pthread_rwlock_unlock(&_vfs_lock);
            return;
        }

        if(avail > 0 && size > 0) {
            char tmp[SHM_PIPE_DATA_SIZE];
            int32_t n = shm_pipe_read(node->shm_ring, tmp, size < avail ? size : avail);
            if(n > 0) {
                sync_pipe_poll_events(node);
                do_node_wakeup(node, VFS_EVT_WR);
                /* Also wake the shm-path writer directly (it bypasses wait
                 * queue). One-shot stamp: consume with exchange (see
                 * read_pipe() in libc). */
                int32_t wpid = __atomic_exchange_n(&node->shm_ring->writer_pid, 0,
                        __ATOMIC_ACQUIRE);
                if(wpid > 0)
                    proc_wakeup_by(wpid, node_id);
                if(shm_pipe_readable(node->shm_ring) == 0 && node->refs_w == 0) {
                    node->events |= VFS_EVT_CLOSE;
                    do_node_wakeup(node, VFS_EVT_CLOSE);
                }
                pthread_rwlock_unlock(&_vfs_lock);
                PF->clear(out)->addi(out, n)->add(out, tmp, n);
                return;
            }
        }
    } else {
        buffer_t* buffer = (buffer_t*)node->data_ptr;
        if(buffer == NULL) {
            sync_pipe_poll_events(node);
            pthread_rwlock_unlock(&_vfs_lock);
            return;
        }

        int32_t unread = buffer->size - buffer->offset;
        if(unread < 0)
            unread = 0;

        if(unread == 0 && node->refs_w == 0) {
            node->events |= VFS_EVT_CLOSE;
            sync_pipe_poll_events(node);
            do_node_wakeup(node, VFS_EVT_CLOSE);
            pthread_rwlock_unlock(&_vfs_lock);
            return;
        }

        if(unread > 0 && size > 0) {
            size = size < unread ? size : unread;
            if(size > 0) {
                PF->clear(out)->addi(out, size)->add(out, buffer->buffer + buffer->offset, size);
                buffer->offset += size;
                if(buffer->offset == buffer->size) {
                    buffer->offset = 0;
                    buffer->size = 0;
                }
                int32_t unread_after = buffer->size - buffer->offset;
                if(unread_after < 0)
                    unread_after = 0;
                if(unread_after == 0 && node->refs_w == 0)
                    node->events |= VFS_EVT_CLOSE;
                sync_pipe_poll_events(node);
                do_node_wakeup(node, VFS_EVT_WR);
                if(unread_after == 0 && node->refs_w == 0)
                    do_node_wakeup(node, VFS_EVT_CLOSE);
                pthread_rwlock_unlock(&_vfs_lock);
                return;
            }
        }
    }

    /*
     * Pipe is empty but writer is still alive. If the caller requested
     * blocking, register the process on the read wait queue atomically.
     *
     * Deliberately NOT stamping shm_ring->reader_pid here - see the mirror
     * comment in do_vfs_pipe_write() for why an un-retirable stamp turns into
     * stale cross-process wakeups. The uuid-validated wait queue below is the
     * only registration this IPC-fallback caller needs; the caller now blocks
     * back through vfs_block(), and a shm-path writer publishes the
     * empty->non-empty edge via vfs_wakeup(node, VFS_EVT_RD).
     */
    if(block) {
        vfs_track_task_slot(pid);
        uint32_t uuid = proc_get_uuid(pid);
        if(uuid != 0)
            enqueue_waiter(&node->read_wait_queue, pid, uuid, false, node_id);
    }

    sync_pipe_poll_events(node);
    pthread_rwlock_unlock(&_vfs_lock);
    PF->clear(out)->addi(out, 0); //retry
}
