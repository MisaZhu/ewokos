/*
 * handlers.c - node/mount/fd-query IPC request handlers and the main
 * command dispatch (handle). Pipe handlers live in pipe.c, process and
 * block/poll handlers in proc_handlers.c.
 */
#include "vfsd.h"

static void do_vfs_get_by_name(int32_t pid, proto_t* in, proto_t* out) {
    (void)pid;
    PF->addi(out, 0);
    const char* name = proto_read_str(in);

    uint32_t node_id = 0;
    if(!vfs_resolve_path(name, &node_id) || node_id == 0)
        return;

    fsinfo_t info;
    pthread_rwlock_rdlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }
    vfs_fill_node_fsinfo(node, &info);
    pthread_rwlock_unlock(&_vfs_lock);

    PF->clear(out)->addi(out, node_id)->add(out, &info, sizeof(fsinfo_t));
}

static void do_vfs_get_by_fd(int pid, proto_t* in, proto_t* out) {
    int fd = proto_read_int(in);
    PF->addi(out, 0);

    fsinfo_t info;
    uint32_t node_id = 0;
    pthread_rwlock_rdlock(&_vfs_lock);
    file_t* file = vfs_check_fd(pid, fd);
    if(file != NULL && file->node != NULL) {
        node_id = vfs_get_node_id(file->node);
        vfs_fill_file_fsinfo(file, &info);
    }
    pthread_rwlock_unlock(&_vfs_lock);

    if(node_id == 0)
        return;
    PF->clear(out)->addi(out, node_id)->add(out, &info, sizeof(fsinfo_t));
}

static void do_vfs_get_by_node(int32_t pid, proto_t* in, proto_t* out) {
    PF->addi(out, 0);
    uint32_t node_id = (uint32_t)proto_read_int(in);
    if(node_id == 0)
        return;

    fsinfo_t info;
    pthread_rwlock_rdlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node == NULL || vfsd_check_access(pid, &node->fsinfo, R_OK) != 0) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }
    vfs_fill_node_fsinfo(node, &info);
    pthread_rwlock_unlock(&_vfs_lock);

    PF->clear(out)->addi(out, node_id)->add(out, &info, sizeof(fsinfo_t));
}

static void do_vfs_new_node(int pid, proto_t* in, proto_t* out) {
    PF->addi(out, -1);
    fsinfo_t info;
    if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
        return;
    uint32_t node_to_id = (uint32_t)proto_read_int(in);
    bool vfs_node_only = (bool)proto_read_int(in);
    bool vfs_write_over = (bool)proto_read_int(in);

    /*
     * The duplicate check below scans the father's kid list, so make sure
     * a lazily-loaded directory is fully mirrored first (sleeps, unlocked).
     */
    if(node_to_id > 0)
        vfs_ensure_kids_loaded(node_to_id);

    pthread_rwlock_wrlock(&_vfs_lock);

    vfs_node_t* node = NULL;
    vfs_node_t* node_to = NULL;
    if(node_to_id > 0) {
        node_to = vfs_get_node_by_id(node_to_id);
        if(node_to == NULL) {
            pthread_rwlock_unlock(&_vfs_lock);
            PF->addi(out, ENOENT);
            return;
        }

        if(!vfs_node_only) {
            if(vfsd_check_access(pid, &node_to->fsinfo, W_OK) != 0 ||
                    vfsd_check_access(pid, &node_to->fsinfo, X_OK) != 0) {
                pthread_rwlock_unlock(&_vfs_lock);
                PF->addi(out, EPERM);
                return;
            }
        }

        node = vfs_find_kid_raw(node_to, info.name);
        if(node != NULL) {//existed !
            if(!vfs_write_over) {
                pthread_rwlock_unlock(&_vfs_lock);
                PF->addi(out, EEXIST);
                return;
            }
        }
        else {
            vfs_write_over = false;
        }
    }

    if(node == NULL) {
        node = vfsd_new_node();
        if(node == NULL) {
            pthread_rwlock_unlock(&_vfs_lock);
            return;
        }
    }

    info.node = vfs_get_node_id(node);
    info.mount_pid = -1;
    memcpy(&node->fsinfo, &info, sizeof(fsinfo_t));
    /*
     * FS_TYPE_PIPE nodes (mkfifo) no longer get any vfsd-side buffer: the
     * in-vfsd pipe backend has been replaced by the standalone pipe driver
     * (system/basic/drivers/piped), which only serves anonymous pipes. The
     * node is still created so paths resolve, but it has no data backend.
     */

    if(node_to != NULL && !vfs_write_over)
        vfs_add_node(pid, node_to, node);

    pthread_rwlock_unlock(&_vfs_lock);
    PF->clear(out)->addi(out, 0)->add(out, &info, sizeof(fsinfo_t));
}

/*
 * Bulk node creation used by mounting filesystems while mirroring the
 * on-disk tree into vfsd: one ipc_call per file made large rootfs mounts
 * painfully slow. Creates a batch of fresh kids under a single father in
 * one round trip and returns the assigned node ids in request order.
 * No overwrite/duplicate handling: the source is an on-disk directory
 * which cannot contain duplicate names, and skipping the per-name scan
 * also avoids an O(n^2) walk of the kid list.
 */
static void do_vfs_new_nodes(int pid, proto_t* in, proto_t* out) {
    PF->addi(out, -1);
    uint32_t node_to_id = (uint32_t)proto_read_int(in);
    int32_t num = proto_read_int(in);
    int32_t sz = 0;
    fsinfo_t* infos = (fsinfo_t*)proto_read(in, &sz);
    if(infos == NULL || num <= 0 || sz < (int32_t)(sizeof(fsinfo_t)*num))
        return;

    pthread_rwlock_wrlock(&_vfs_lock);

    vfs_node_t* node_to = vfs_get_node_by_id(node_to_id);
    if(node_to == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        PF->addi(out, ENOENT);
        return;
    }

    if(vfsd_check_access(pid, &node_to->fsinfo, W_OK) != 0 ||
            vfsd_check_access(pid, &node_to->fsinfo, X_OK) != 0) {
        pthread_rwlock_unlock(&_vfs_lock);
        PF->addi(out, EPERM);
        return;
    }

    uint32_t* ids = (uint32_t*)malloc(sizeof(uint32_t)*num);
    if(ids == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    for(int32_t i=0; i<num; i++) {
        vfs_node_t* node = vfsd_new_node();
        if(node == NULL) {
            free(ids);
            pthread_rwlock_unlock(&_vfs_lock);
            return;
        }
        infos[i].node = vfs_get_node_id(node);
        infos[i].mount_pid = -1;
        memcpy(&node->fsinfo, &infos[i], sizeof(fsinfo_t));
        vfs_add_node(pid, node_to, node);
        ids[i] = infos[i].node;
    }
    pthread_rwlock_unlock(&_vfs_lock);

    PF->clear(out)->addi(out, 0)->add(out, ids, sizeof(uint32_t)*num);
    free(ids);
}

static void do_vfs_open(int32_t pid, proto_t* in, proto_t* out) {
    PF->addi(out, -1);
    fsinfo_t info;
    if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
        return;

    int32_t flags = proto_read_int(in);

    int32_t res = -1;
    fsinfo_t out_info;
    bool have_info = false;

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(info.node);
    if(node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        PF->addi(out, ENOENT);
        return;
    }

    if(((flags & O_WRONLY) != 0 ||
            (flags & O_RDWR) != 0) &&
            vfsd_check_access(pid, &node->fsinfo, W_OK) != 0) {
        pthread_rwlock_unlock(&_vfs_lock);
        PF->addi(out, EPERM);
        return;
    }

    /*
     * Permission checks must match the requested access mode:
     * - O_RDONLY: require read
     * - O_WRONLY: require write only
     * - O_RDWR:   require both
     *
     * The previous logic always required R_OK as well, which made pure
     * write opens fail for non-root users on write-only targets.
     */
    if((flags & O_WRONLY) == 0 &&
            vfsd_check_access(pid, &node->fsinfo, R_OK) != 0) {
        pthread_rwlock_unlock(&_vfs_lock);
        PF->addi(out, EPERM);
        return;
    }

    if(!FS_IS_ANONYMOUS(node->fsinfo.type))
        res = vfsd_open(pid, node, flags);
    else {
        node = vfs_open_announimous(pid, node);
        if(node != NULL)
            res = vfsd_open(pid, node, flags);
    }

    if(res >= 0) {
        vfs_fill_node_fsinfo(node, &out_info);
        have_info = true;
        file_t* file = vfs_check_fd(pid, res);
        if(file != NULL)
            vfs_fill_file_fsinfo(file, &out_info);
    }
    pthread_rwlock_unlock(&_vfs_lock);

    if(res < 0) { //keep the [res][errno] error reply shape.
        PF->clear(out);
        PF->addi(out, -1)->addi(out, ENFILE);
        return;
    }

    PF->clear(out);
    PF->addi(out, res);
    if(have_info)
        PF->add(out, &out_info, sizeof(fsinfo_t));
}

static void do_vfs_close(int32_t pid, proto_t* in) {
    int fd = proto_read_int(in);
    if(fd < 0)
        return;
    pthread_rwlock_wrlock(&_vfs_lock);
    vfsd_close(pid, fd);
    pthread_rwlock_unlock(&_vfs_lock);
}

/*
 * Same-process dup/dup2: only pipe fds notify the driver (piped counts
 * descriptors). The notification is synchronous so the driver ref exists
 * before the caller can close anything on this pipe; if it fails the new
 * fd's driver_ref is dropped again to keep exit-time CLOSE accounting
 * balanced.
 */
static void do_vfs_dup(int32_t pid, proto_t* in, proto_t* out) {
    int fd = proto_read_int(in);
    int32_t fdto = -1;
    file_t dup_file;
    bool have_file = false;
    int32_t owner = -1;

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node = vfsd_dup(pid, fd, &fdto);
    if(node != NULL) {
        owner = vfs_fd_owner_pid(pid);
        file_t* file = vfs_check_fd(pid, fdto);
        if(file != NULL) {
            dup_file = *file;
            have_file = true;
        }
    }
    pthread_rwlock_unlock(&_vfs_lock);

    if(node == NULL) {
        PF->addi(out, -1);
        return;
    }

    if(have_file) {
        if(vfs_driver_dup(owner, fd, owner, fdto, &dup_file) != 0)
            vfs_clear_fd_driver_ref(owner, fdto, dup_file.node);
    }

    fsinfo_t out_info;
    bool have_info = false;
    pthread_rwlock_rdlock(&_vfs_lock);
    file_t* file = vfs_check_fd(pid, fdto);
    if(file != NULL)
        have_info = (vfs_fill_file_fsinfo(file, &out_info) == 0);
    pthread_rwlock_unlock(&_vfs_lock);

    if(!have_info) {
        PF->addi(out, -1);
        return;
    }
    PF->addi(out, fdto)->add(out, &out_info, sizeof(fsinfo_t));
}

static void do_vfs_dup2(int32_t pid, proto_t* in, proto_t* out) {
    int32_t fd = proto_read_int(in);
    int32_t fdto = proto_read_int(in);
    file_t dup_file;
    bool have_file = false;
    int32_t owner = -1;
    driver_close_task_t* pipe_victim = NULL;

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node = vfsd_dup2(pid, fd, fdto, &pipe_victim);
    if(node != NULL) {
        owner = vfs_fd_owner_pid(pid);
        file_t* file = vfs_check_fd(pid, fdto);
        if(file != NULL) {
            dup_file = *file;
            have_file = true;
        }
    }
    pthread_rwlock_unlock(&_vfs_lock);

    /*
     * A pipe victim is closed synchronously BEFORE the new end's FS_CMD_DUP:
     * the dup2 overwrite is the one spot where a -1 must land before its
     * paired +1, otherwise piped's refcount transiently shows an extra live
     * descriptor. Non-pipe victims stay on the async queue (netd sockets may
     * stall; dup2 of a socket must not wedge on them).
     */
    if(pipe_victim != NULL) {
        vfs_driver_close(pipe_victim->pid, pipe_victim->owner_pid,
                pipe_victim->fd, &pipe_victim->file);
        free(pipe_victim);
    }

    if(node == NULL) {
        PF->addi(out, -1);
        return;
    }

    if(have_file) {
        if(vfs_driver_dup(owner, fd, owner, fdto, &dup_file) != 0)
            vfs_clear_fd_driver_ref(owner, fdto, dup_file.node);
    }

    fsinfo_t out_info;
    bool have_info = false;
    pthread_rwlock_rdlock(&_vfs_lock);
    file_t* file = vfs_check_fd(pid, fdto);
    if(file != NULL)
        have_info = (vfs_fill_file_fsinfo(file, &out_info) == 0);
    pthread_rwlock_unlock(&_vfs_lock);

    if(!have_info) {
        PF->addi(out, -1);
        return;
    }
    PF->addi(out, fdto)->add(out, &out_info, sizeof(fsinfo_t));
}

static void do_vfs_set_fsinfo(int32_t pid, proto_t* in, proto_t* out) {
    fsinfo_t info;
    PF->addi(out, -1);
    int res = -1;
    if(proto_read_to(in, &info, sizeof(fsinfo_t)) == sizeof(fsinfo_t)) {
        pthread_rwlock_wrlock(&_vfs_lock);
        vfs_node_t* node = vfs_get_node_by_id(info.node);
        if(node == NULL) {
            pthread_rwlock_unlock(&_vfs_lock);
            PF->addi(out, ENOENT);
            return;
        }
        if(vfsd_check_access(pid, &node->fsinfo, W_OK) != 0) {
            pthread_rwlock_unlock(&_vfs_lock);
            PF->addi(out, EPERM);
            return;
        }
        res = set_node_info(pid, node, &info);
        pthread_rwlock_unlock(&_vfs_lock);
    }
    PF->clear(out)->addi(out, res);
}

static void do_vfs_set_by_fd(int32_t pid, proto_t* in, proto_t* out) {
    int fd = proto_read_int(in);
    fsinfo_t info;
    PF->addi(out, -1);
    if(proto_read_to(in, &info, sizeof(fsinfo_t)) != sizeof(fsinfo_t))
        return;

    int32_t res = -1;
    pthread_rwlock_wrlock(&_vfs_lock);
    file_t* file = vfs_check_fd(pid, fd);
    if(file != NULL && file->node != NULL) {
        memcpy(&file->fsinfo, &info, sizeof(fsinfo_t));
        file->fsinfo.node = vfs_get_node_id(file->node);
        file->fsinfo.mount_pid = get_mount_pid(file->node);
        res = 0;
    }
    pthread_rwlock_unlock(&_vfs_lock);
    if(res == 0)
        PF->clear(out)->addi(out, 0);
}

static void do_vfs_get_kids(int pid, proto_t* in, proto_t* out) {
    PF->addi(out, 0);
    uint32_t node_id = (uint32_t)proto_read_int(in);
    if(node_id == 0)
        return;

    pthread_rwlock_rdlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node == NULL || vfsd_check_access(pid, &node->fsinfo, X_OK) != 0) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }
    pthread_rwlock_unlock(&_vfs_lock);

    uint32_t num = 0;
    fsinfo_t* kids = vfs_get_kids(node_id, &num);
    if(kids == NULL || num == 0)
        return;
    PF->clear(out)->addi(out, num)->add(out, kids, sizeof(fsinfo_t)*num);
    free(kids);
}

static void do_vfs_del_node(int32_t pid, proto_t* in, proto_t* out) {
    (void)pid;
    PF->addi(out, -1);
    uint32_t node_id = proto_read_int(in);
    if(node_id == 0)
        return;

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }
    int res = vfsd_del_node(node);
    pthread_rwlock_unlock(&_vfs_lock);
    PF->clear(out)->addi(out, res);
}

static void do_vfs_mount(int32_t pid, proto_t* in, proto_t* out) {
    uint32_t node_to_id;
    uint32_t node_id;

    PF->addi(out, -1);
    node_to_id = (uint32_t)proto_read_int(in);
    if(node_to_id == 0)
        return;
    node_id = (uint32_t)proto_read_int(in);
    if(node_id == 0)
        return;

    const char* desc = proto_read_str(in);

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node_to = vfs_get_node_by_id(node_to_id);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node_to == NULL || node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    vfsd_mount(pid, node_to, node, desc);
    pthread_rwlock_unlock(&_vfs_lock);
    PF->clear(out)->addi(out, 0);
}

static void do_vfs_umount(int32_t pid, proto_t* in) {
    uint32_t node_id = (uint32_t)proto_read_int(in);
    if(node_id == 0)
        return;

    pthread_rwlock_wrlock(&_vfs_lock);
    vfs_node_t* node = vfs_get_node_by_id(node_id);
    if(node == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return;
    }

    vfsd_umount(pid, node);
    pthread_rwlock_unlock(&_vfs_lock);
}

static void do_vfs_get_mount_by_id(proto_t* in, proto_t* out) {
    mount_t mount;
    pthread_rwlock_rdlock(&_vfs_lock);
    int32_t res = vfsd_get_mount_by_id(proto_read_int(in), &mount);
    pthread_rwlock_unlock(&_vfs_lock);
    if(res != 0) {
        PF->addi(out, -1);
        return;
    }
    PF->addi(out, 0)->add(out, &mount, sizeof(mount_t));
}

void handle(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
    (void)p;
    if(pid < 0)
        return;
    vfs_drain_driver_kids_results();

    switch(cmd) {
    case VFS_NEW_NODE:
        do_vfs_new_node(pid, in, out);
        break;
    case VFS_NEW_NODES:
        do_vfs_new_nodes(pid, in, out);
        break;
    case VFS_OPEN:
        do_vfs_open(pid, in, out);
        break;
    /* VFS_PIPE_OPEN/VFS_PIPE_WRITE/VFS_PIPE_READ ids are intentionally kept
     * unused (no case here): anonymous pipes now live entirely inside the
     * standalone pipe driver, and reusing the ids would shift the protocol. */
    case VFS_DUP:
        do_vfs_dup(pid, in, out);
        break;
    case VFS_DUP2:
        do_vfs_dup2(pid, in, out);
        break;
    case VFS_CLOSE:
        do_vfs_close(pid, in);
        break;
    case VFS_GET_BY_NAME:
        do_vfs_get_by_name(pid, in, out);
        break;
    case VFS_GET_BY_NODE:
        do_vfs_get_by_node(pid, in, out);
        break;
    case VFS_GET_BY_FD:
        do_vfs_get_by_fd(pid, in, out);
        break;
    case VFS_SET_FSINFO:
        do_vfs_set_fsinfo(pid, in, out);
        break;
    case VFS_DEL_NODE:
        do_vfs_del_node(pid, in, out);
        break;
    case VFS_MOUNT:
        do_vfs_mount(pid, in, out);
        break;
    case VFS_UMOUNT:
        do_vfs_umount(pid, in);
        break;
    case VFS_GET_KIDS:
        do_vfs_get_kids(pid, in, out);
        break;
    case VFS_GET_MOUNT_BY_ID:
        do_vfs_get_mount_by_id(in, out);
        break;
    case VFS_PROC_CLONE:
        do_vfs_proc_clone(pid, in);
        break;
    case VFS_PROC_EXIT:
        do_vfs_proc_exit(pid, in);
        break;
    case VFS_BLOCK:
        do_vfs_block(pid, in);
        break;
    case VFS_WAKEUP:
        do_vfs_wakeup(pid, in);
        break;
    case VFS_UNBLOCK:
        do_vfs_unblock(pid, in);
        break;
    case VFS_GET_POLL_EVENTS:
        do_vfs_get_poll_events(pid, in, out);
        break;
    case VFS_CLEAR_POLL_EVENTS:
        do_vfs_clear_poll_events(pid, in, out);
        break;
    case VFS_SET_BY_FD:
        do_vfs_set_by_fd(pid, in, out);
        break;
    default:
        break;
    }
}
