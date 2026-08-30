/*
 * fd.c - per-process fd table, open/close/dup/dup2 and fsinfo snapshots.
 */
#include "vfsd.h"

proc_fds_t* _proc_fds_table = NULL;
uint32_t    _max_proc_table_num = 0;

#define VFSD_BACKUP_FD0 (MAX_OPEN_FILE_PER_PROC-3)
#define VFSD_BACKUP_FD1 (MAX_OPEN_FILE_PER_PROC-2)

int32_t vfs_fd_owner_pid(int32_t pid) {
    int32_t owner = proc_getpid(pid);
    if(owner < 0)
        owner = pid;
    return owner;
}

/* caller must hold _vfs_lock when dereferencing the returned slot */
file_t* vfs_get_file(int32_t pid, int32_t fd) {
    if(pid < 0 || (uint32_t)pid >= _max_proc_table_num || fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
        return NULL;
    return &_proc_fds_table[pid].fds[fd];
}

/* caller must hold _vfs_lock (read or write) */
file_t* vfs_check_fd(int32_t pid, int32_t fd) {
    int32_t owner = vfs_fd_owner_pid(pid);
    file_t* f = vfs_get_file(owner, fd);
    if(f == NULL || f->node == NULL)
        return NULL;
    return f;
}

/* caller must hold _vfs_lock (read or write) */
static int32_t get_free_fd(int32_t pid) {
    pid = vfs_fd_owner_pid(pid);
    if(pid < 0 || pid >= (int32_t)_max_proc_table_num)
        return -1;
    int32_t i;
    for(i=3; i<MAX_OPEN_FILE_PER_PROC; i++) { //0, 1, 2 reserved for stdio in/out/err
        if(_proc_fds_table[pid].fds[i].node == 0)
            return i;
    }
    return -1;
}

/* caller must hold _vfs_lock (write) */
int32_t vfsd_open(int32_t pid, vfs_node_t* node, int32_t flags) {
    int32_t owner = vfs_fd_owner_pid(pid);
    if(node == NULL)
        return -1;
    if(owner < 0 || (uint32_t)owner >= _max_proc_table_num)
        return -1;
    _proc_fds_table[owner].owner_pid = owner;

    int32_t fd = get_free_fd(owner);
    if(fd < 0)
        return -1;

    _proc_fds_table[owner].fds[fd].node = node;
    _proc_fds_table[owner].fds[fd].flags = flags;
    _proc_fds_table[owner].fds[fd].driver_ref = 1;
    memcpy(&_proc_fds_table[owner].fds[fd].fsinfo, &node->fsinfo, sizeof(fsinfo_t));
    _proc_fds_table[owner].fds[fd].fsinfo.node = vfs_get_node_id(node);
    _proc_fds_table[owner].fds[fd].fsinfo.mount_pid = get_mount_pid(node);

    if((flags & O_TRUNC) != 0)
        node->fsinfo.stat.size = 0;

    node->refs++;
    if((flags & (O_WRONLY | O_RDWR)) != 0)
        node->refs_w++;
    return fd;
}

/* caller must hold _vfs_lock (write) */
vfs_node_t* vfs_open_announimous(int32_t pid, vfs_node_t* node) {
    if(node == NULL)
        return NULL;

    procinfo_t procinfo;
    if(proc_info(pid, &procinfo) != 0)
        return NULL;

    vfs_node_t* ret = vfsd_new_node();
    if(ret == NULL)
        return NULL;
    ret->fsinfo.type = node->fsinfo.type;
    ret->fsinfo.stat.mode = 0700;
    ret->fsinfo.stat.uid = procinfo.uid;
    ret->fsinfo.stat.gid = procinfo.gid;
    ret->fsinfo.data = 0;
    ret->mount_id = node->mount_id;
    return ret;
}

/* caller must hold _vfs_lock (read or write) */
vfs_node_t* vfsd_get_by_fd(int32_t pid, int32_t fd) {
    file_t* file = vfs_check_fd(pid, fd);
    if(file == NULL)
        return NULL;
    return file->node;
}

/*
 * Fill a fsinfo snapshot of a live node. Replaces the old static-buffer
 * gen_fsinfo() which raced between concurrent handlers.
 * caller must hold _vfs_lock (read or write).
 */
void vfs_fill_node_fsinfo(vfs_node_t* node, fsinfo_t* out) {
    if(node == NULL || out == NULL)
        return;
    memcpy(out, &node->fsinfo, sizeof(fsinfo_t));
    out->node = vfs_get_node_id(node);
    out->mount_pid = get_mount_pid(node);
}

/*
 * Fill a fsinfo snapshot of an open fd. Replaces the old static-buffer
 * gen_file_fsinfo(). caller must hold _vfs_lock (read or write).
 *
 * The fd-side type comes from file->fsinfo, NOT the node: drivers advertise
 * a per-fd type through VFS_SET_BY_FD after open (piped: FS_TYPE_PIPE on an
 * anonymous CHAR node). It is initialized from the node type at open/dup, so
 * for every other device it stays identical to node->fsinfo.type.
 */
int32_t vfs_fill_file_fsinfo(file_t* file, fsinfo_t* out) {
    if(file == NULL || file->node == NULL || out == NULL)
        return -1;
    memcpy(out, &file->node->fsinfo, sizeof(fsinfo_t));
    out->type = file->fsinfo.type;
    out->data = file->fsinfo.data;
    out->node = vfs_get_node_id(file->node);
    out->mount_pid = get_mount_pid(file->node);
    return 0;
}

/* caller must hold _vfs_lock (write) */
void vfsd_close(int32_t pid, int32_t fd) {
    int32_t owner = vfs_fd_owner_pid(pid);
    if(pid < 0 || fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
        return;
    if(owner < 0 || (uint32_t)owner >= _max_proc_table_num)
        return;
    _proc_fds_table[owner].owner_pid = owner;

    file_t* f = vfs_get_file(owner, fd);
    if(f != NULL && f->node != NULL) {
        /*
         * Do NOT send FS_CMD_CLOSE here — the user-space vfsd_close() already
         * sends FS_CMD_CLOSE directly to the device driver before calling
         * VFS_CLOSE on vfsd. Adding another would double-decrement the
         * driver's per-socket reference count, causing premature release.
         *
         * Device-close notification for process exit is handled exclusively
         * in clear_zombie(), where there is no user-space path.
         */
        proc_file_close(owner, fd, f);
        memset(f, 0, sizeof(file_t));
    }
}

/* caller must hold _vfs_lock (write) */
vfs_node_t* vfsd_dup(int32_t pid, int32_t from, int32_t *ret) {
    int32_t owner = vfs_fd_owner_pid(pid);
    if(from < 0 || from > MAX_OPEN_FILE_PER_PROC)
        return NULL;
    if(owner < 0 || (uint32_t)owner >= _max_proc_table_num)
        return NULL;
    int32_t to = get_free_fd(owner);
    if(to < 0)
        return NULL;

    file_t* f = vfs_check_fd(owner, from);
    if(f == NULL)
        return NULL;

    file_t* f_to = vfs_get_file(owner, to);
    if(f_to == NULL)
        return NULL;

    memcpy(f_to, f, sizeof(file_t));
    /* Same-process dup() never sends FS_CMD_DUP, so the new fd owns no
     * driver-side reference; the surviving source fd keeps it.
     * Pipes are the exception: piped counts descriptors, and the dup'd end
     * can outlive its source (dup2(pipe,1); close(pipe)) — without its own
     * ref the pipe end would be declared closed while the dup'd descriptor
     * is still open. vfs_driver_dup() below ships the matching FS_CMD_DUP. */
    f_to->driver_ref = FS_IS_TYPE(f->fsinfo.type, FS_TYPE_PIPE) ? 1 : 0;
    f->node->refs++;
    if((f->flags & (O_WRONLY | O_RDWR)) != 0)
        f->node->refs_w++;
    *ret = to;
    return f_to->node;
}

/* caller must hold _vfs_lock (write).
 * pipe_victim: optional out-param. When the overwritten fd is a pipe end,
 * its driver close task is handed back instead of queued — the handler
 * ships it synchronously before the new end's FS_CMD_DUP so piped never
 * observes the +1 before the paired -1. NULL for non-pipe victims (they
 * keep the async queue). */
vfs_node_t* vfsd_dup2(int32_t pid, int32_t from, int32_t to,
        driver_close_task_t** pipe_victim) {
    int32_t owner = vfs_fd_owner_pid(pid);
    file_t* f = vfs_check_fd(owner, from);
    if(f == NULL)
        return NULL;

    if(from == to)
        return f->node;

    if(from < 0 || from > MAX_OPEN_FILE_PER_PROC ||
            to < 0 || to > MAX_OPEN_FILE_PER_PROC)
        return NULL;
    if(owner < 0 || (uint32_t)owner >= _max_proc_table_num)
        return NULL;

    /*
     * dup2 atomically replaces the target fd; user space never gets a
     * chance to close() the overwritten file, so unlike the explicit
     * vfsd_close() path (where libc notifies the driver first), no
     * FS_CMD_CLOSE reaches the driver for the old target. If it owned
     * a driver-side reference (driver_ref=1), return it now — otherwise
     * the driver's per-fd ref leaks. A telnet shell's pipeline child
     * dup2()ing a pipe over its inherited socket fd 0 (driver_ref=1 from
     * fork) leaked the netd connection ref, keeping the connection task
     * alive after exit.
     */
    file_t* f_old = vfs_get_file(owner, to);
    if(f_old != NULL && f_old->node != NULL) {
        uint32_t type = FS_BASE_TYPE(f_old->fsinfo.type);
        if(type != FS_TYPE_FILE &&
                type != FS_TYPE_DIR &&
                type != FS_TYPE_LINK &&
                f_old->driver_ref &&
                f_old->fsinfo.mount_pid > 0 &&
                f_old->fsinfo.node != 0) {
            driver_close_task_t* task =
                (driver_close_task_t*)malloc(sizeof(driver_close_task_t));
            if(task != NULL) {
                task->pid = owner;
                task->owner_pid = owner;
                task->fd = to;
                task->file = *f_old;
                if(FS_IS_TYPE(f_old->fsinfo.type, FS_TYPE_PIPE) &&
                        pipe_victim != NULL) {
                    task->job_type = DRIVER_ASYNC_JOB_CLOSE;
                    *pipe_victim = task;
                } else {
                    enqueue_driver_close_task(task);
                }
            }
        }
    }
    vfsd_close(owner, to);
    file_t* f_to = vfs_get_file(owner, to);
    if(f_to == NULL)
        return NULL;

    memcpy(f_to, f, sizeof(file_t));
    /* Same-process dup2() never sends FS_CMD_DUP, so the new fd owns no
     * driver-side reference; the surviving source fd keeps it.
     * Pipes excepted — see vfsd_dup() above. */
    f_to->driver_ref = FS_IS_TYPE(f->fsinfo.type, FS_TYPE_PIPE) ? 1 : 0;
    f->node->refs++;
    if((f->flags & (O_WRONLY | O_RDWR)) != 0)
        f->node->refs_w++;
    return f->node;
}

/*
 * Undo the driver_ref mark when the matching FS_CMD_DUP never reached the
 * driver: without this, clear_zombie() would ship an unmatched FS_CMD_CLOSE
 * at exit and drive piped's descriptor refcount below the true count.
 * 'node' re-validates the slot: the fd may have been closed/reused between
 * the failed dup and this call. Must be called with NO _vfs_lock held.
 */
void vfs_clear_fd_driver_ref(int32_t pid, int32_t fd, const vfs_node_t* node) {
    if(pid < 0 || fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC || node == NULL)
        return;
    pthread_rwlock_wrlock(&_vfs_lock);
    file_t* f = vfs_get_file(pid, fd);
    if(f != NULL && f->node == node)
        f->driver_ref = 0;
    pthread_rwlock_unlock(&_vfs_lock);
}

/*
 * Snapshot a directory's kid list into a fresh array. Manages _vfs_lock
 * itself (the kids-load step may sleep unlocked). Returns NULL with *num=0
 * when the node vanished or has no kids.
 */
fsinfo_t* vfs_get_kids(uint32_t node_id, uint32_t* num) {
    *num = 0;
    if(node_id == 0)
        return NULL;

    vfs_ensure_kids_loaded(node_id);

    pthread_rwlock_rdlock(&_vfs_lock);
    vfs_node_t* father = vfs_get_node_by_id(node_id);
    if(father == NULL || father->kids_num == 0) {
        pthread_rwlock_unlock(&_vfs_lock);
        return NULL;
    }

    uint32_t n = father->kids_num;
    fsinfo_t* ret = (fsinfo_t*)malloc(n * sizeof(fsinfo_t));
    if(ret == NULL) {
        pthread_rwlock_unlock(&_vfs_lock);
        return NULL;
    }

    uint32_t i = 0;
    vfs_node_t* node = father->first_kid;
    while(node != NULL && i<n) {
        vfs_fill_node_fsinfo(node, &ret[i]);
        node = node->next;
        i++;
    }
    pthread_rwlock_unlock(&_vfs_lock);

    *num = i;
    return ret;
}
