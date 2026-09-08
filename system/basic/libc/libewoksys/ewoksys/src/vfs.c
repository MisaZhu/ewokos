#include <ewoksys/vfs.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ewoksys/mstr.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/devcmd.h>
#include <ewoksys/proc.h>
#include <ewoksys/shm_pipe.h>
#include <ewoksys/thread.h>
#include <sys/shm.h>
// #include <sys/unistd.h>
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal wire-protocol constants for the standalone pipe driver. The
 * canonical definitions live in system/basic/drivers/piped/piped.h; libc
 * deliberately keeps no piped-specific header — only these values and the
 * shared data-plane ring (ewoksys/shm_pipe.h). Keep both sides in sync.
 */
#define PIPE_DEV_PATH      "/dev/pipe0"
#define PIPE_ATTACH        0x100
#define PIPE_POLL_WAIT     0x101
#define PIPE_POLL_UNWAIT   0x102
#define PIPE_EDGE          0x103


static fsfile_t _fsfiles[MAX_OPEN_FILE_PER_PROC];
static shm_pipe_t* _pipe_shm[MAX_OPEN_FILE_PER_PROC];
static uint32_t vfs_get_poll_events_by_node(ewokos_addr_t node_id);

typedef struct {
        fsinfo_t info;
        bool valid;
        /* true when this fd parked through piped (PIPE_POLL_WAIT) instead of
         * vfsd's vfs_block_raw; cleanup must go back to piped too. */
        bool pipe_wait;
} poll_fd_cache_t;

#define VFS_MULTI_POLL_BACKOFF_US 20000
/*
 * Backoff for poll() waits that cannot park on a single node token.
 *
 * vfsd keeps only one read and one write waiter per pid, so a multi-fd
 * poll() registering on several nodes leaves only the LAST registration
 * armed — events on the evicted nodes can never wake the block. An
 * infinite timeout therefore must not block forever: it sleeps on this
 * bounded deadline and re-checks every fd level-triggered each round.
 *
 * Ramp instead of a flat sleep: catch the common sub-millisecond refill
 * on the first retry (a flat 10ms/20ms would cap a relay such as
 * sshd/telnetd shell output near 4KB per period — the shm_pipe ring is
 * 4032 bytes), and still reach the ceiling within ~25ms so a genuinely
 * idle waiter does not flood vfsd with readiness IPCs.
 */
#define VFS_POLL_BACKOFF_START_US 200U

static int vfs_get_by_fd_raw(int fd, fsinfo_t* info) {
    proto_t in, out;
    PF->init(&in)->addi(&in, fd);
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_GET_BY_FD, &in, &out);
    PF->clear(&in);

    if(res == 0) {
        res = proto_read_int(&out); //res = node
        if(res != 0) {
            if(info != NULL)
                proto_read_to(&out, info, sizeof(fsinfo_t));
            res = 0;
        }
        else
            res = -1;
    }
    PF->clear(&out);
    return res;
}

static int vfs_set_info(fsinfo_t* info) {
    proto_t in, out;
    PF->init(&in)->add(&in, info, sizeof(fsinfo_t));
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_SET_FSINFO, &in, &out);
    PF->clear(&in);

    if(res == 0) {
        res = proto_read_int(&out);
    }
    PF->clear(&out);
    return res;
}

void  vfs_init(void) {
    for(uint32_t i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
        memset(&_fsfiles[i], 0, sizeof(fsfile_t));
        _pipe_shm[i] = NULL;
    }
}

void  vfs_on_fork(void) {
    /* After fork the child inherits _pipe_shm[] pointers via COW but the
     * shared memory pages are not mapped in the child's page table.
     * Clear all entries so get_pipe_shm() will re-map via shmat() lazily. */
    for(uint32_t i=0; i<MAX_OPEN_FILE_PER_PROC; i++)
        _pipe_shm[i] = NULL;
    /* Same story for the per-fd IO transfer-buffer cache in devcmd.c. */
    dev_io_on_fork();
}

static inline fsfile_t* vfs_set_file(int fd, fsinfo_t* info) {
    if(fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
        return NULL;
    fsfile_t* file = &_fsfiles[fd];
    memcpy(&file->info, info, sizeof(fsinfo_t));
    return file;
}

static inline void vfs_update_file(fsinfo_t* info) {
    /*
     * Anonymous single-node char devices (e.g. /dev/net0, xserverd) keep
     * fd-private per-open state in fsinfo.data (netd: its task pointer),
     * but every socket/client shares ONE node. Broadcasting one fd's
     * fsinfo to every fd on the same node clobbers the siblings' data
     * pointers: a later close() then releases another socket's task (its
     * own task leaks -> orphaned netd worker thread) and dereferences the
     * freed task again on the next close (netd data abort in
     * network_close). File/pipe nodes describe one object per node, so
     * the broadcast remains valid for them.
     */
    if(FS_IS_ANONYMOUS(info->type) && FS_IS_TYPE(info->type, FS_TYPE_CHAR))
        return;
    for(uint32_t i=0; i<MAX_OPEN_FILE_PER_PROC; i++) {
        if(_fsfiles[i].info.node == info->node)
            memcpy(&_fsfiles[i].info, info, sizeof(fsinfo_t));
    }
}

static inline void vfs_clear_file(int fd) {
    if(fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
        return;
    memset(&_fsfiles[fd], 0, sizeof(fsfile_t));
    _pipe_shm[fd] = NULL; /* don't shmdt — other fds may share the same ring */
    dev_io_on_close(fd); /* free this fd's private IO transfer buffer */
}

static fsfile_t* vfs_get_file(int fd) {
    if(fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
        return NULL;

    fsfile_t* ret = &_fsfiles[fd];
    if(ret->info.node != 0)
        return ret;

    fsinfo_t info;
    if(vfs_get_by_fd_raw(fd, &info) != 0)
        return NULL;
    return vfs_set_file(fd, &info);
}

int vfs_get_flags(int fd) {
    fsfile_t* file = vfs_get_file(fd);
    if(file == NULL)
        return -1;
    return file->flags;
}

int vfs_set_flags(int fd, int flags) {
    fsfile_t* file = vfs_get_file(fd);
    if(file == NULL)
        return -1;
    file->flags = flags;
    return 0;
}

int vfs_get_fd_flags(int fd) {
    fsfile_t* file = vfs_get_file(fd);
    if(file == NULL)
        return -1;
    return file->fd_flags;
}

int vfs_set_fd_flags(int fd, int flags) {
    fsfile_t* file = vfs_get_file(fd);
    if(file == NULL)
        return -1;
    file->fd_flags = flags;
    return 0;
}

int vfs_check_access(int pid, fsinfo_t* info, int mode) {
    procinfo_t procinfo;
    if(info == NULL || proc_info(pid, &procinfo) != 0)
    return -1;

    if(procinfo.uid <= 0) {
        if(mode == X_OK && (info->stat.mode & 0111) == 0)
            return -1;
        return 0;
    }

    int ucheck = 0400;	
    int gcheck = 040;	
    int acheck = 04;	
    if(mode == R_OK) {
        ucheck = 0400;	
        gcheck = 040;	
        acheck = 04;	
    }
    else if(mode == W_OK) {
        ucheck = 0200;	
        gcheck = 020;	
        acheck = 02;	
    }
    else if(mode == X_OK) {
        ucheck = 0100;	
        gcheck = 010;	
        acheck = 01;	
    }

    if(procinfo.uid == info->stat.uid) {
        if((info->stat.mode & ucheck) != 0)
            return 0;
    }
    else if(procinfo.gid == info->stat.gid) {
        if((info->stat.mode & gcheck) != 0)
            return 0;
    }
    else {
        if((info->stat.mode & acheck) != 0)
            return 0;
    }
    return -1;
}

int vfs_get_by_fd(int fd, fsinfo_t* info) {
    fsfile_t* buffer = vfs_get_file(fd);
    if(buffer != NULL) {
        memcpy(info, &buffer->info, sizeof(fsinfo_t));
        return 0;
    }
        return -1;
}

int vfs_set_by_fd(int fd, fsinfo_t* info) {
    proto_t in, out;
    PF->format(&in, "i,m", (ewokos_addr_t)fd, info, sizeof(fsinfo_t));
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_SET_BY_FD, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        res = proto_read_int(&out);
        if(res == 0) {
            vfs_set_file(fd, info);
        }
    }
    PF->clear(&out);
    return res;
}

int vfs_get_by_node(ewokos_addr_t node, fsinfo_t* info) {
    proto_t in, out;
    PF->init(&in)->addi(&in, node);
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_GET_BY_NODE, &in, &out);
    PF->clear(&in);

    if(res == 0) {
        res = proto_read_int(&out); //res = node
        if(res != 0) {
            if(info != NULL)
                proto_read_to(&out, info, sizeof(fsinfo_t));
            res = 0;
        }
        else
            res = -1;
    }
    PF->clear(&out);
    return res;
}

int vfs_new_node(fsinfo_t* info, ewokos_addr_t node_to, bool vfs_node_only, bool vfs_write_over) {
    proto_t in, out;
    PF->format(&in, "m,i,i,i", info, sizeof(fsinfo_t), node_to,
            vfs_node_only, vfs_write_over);
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_NEW_NODE, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        res = proto_read_int(&out); //res = node
        if(res == 0)
            proto_read_to(&out, info, sizeof(fsinfo_t));
        else
            res = -1;
    }
    PF->clear(&out);
    return res;	
}

/*
 * Bulk version of vfs_new_node for mounting filesystems: registering the
 * whole on-disk tree one synchronous ipc_call per file dominates mount
 * time, so a batch of fresh kids under one father is sent in a single
 * round trip. On success the assigned node ids are written back into
 * each infos[i].node.
 */
int vfs_new_nodes(fsinfo_t* infos, uint32_t num, ewokos_addr_t node_to) {
    if(infos == NULL || num == 0)
        return -1;

    proto_t in, out;
    PF->format(&in, "i,i,m", node_to, (ewokos_addr_t)num, infos,
            (int32_t)(sizeof(fsinfo_t)*num));
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_NEW_NODES, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        res = proto_read_int(&out);
        if(res == 0) {
            int32_t sz = 0;
            ewokos_addr_t* ids = (ewokos_addr_t*)proto_read(&out, &sz);
            if(ids != NULL && sz >= (int32_t)(sizeof(ewokos_addr_t)*num)) {
                for(uint32_t i=0; i<num; i++)
                    infos[i].node = ids[i];
            }
            else
                res = -1;
        }
        else
            res = -1;
    }
    PF->clear(&out);
    return res;
}

void vfs_fullname(const char* fname, char* ret, uint32_t len) {
    if(fname[0] == '/') {
        strncpy(ret, fname, len);
        return;
    }

    str_t* fullname = str_new("");
    char pwd[FS_FULL_NAME_MAX+1] = {0};
    getcwd(pwd, FS_FULL_NAME_MAX);
    str_cpy(fullname, pwd);
    if(pwd[1] != 0)
        str_addc(fullname, '/');
    str_add(fullname, fname);

    strncpy(ret, fullname->cstr, FS_FULL_NAME_MAX);
    str_free(fullname);
}

int vfs_open(fsinfo_t* info, int oflag) {
    proto_t in, out;
    PF->format(&in, "m,i", info, sizeof(fsinfo_t), (ewokos_addr_t)oflag);
    //PF->init(&in)->add(&in, info, sizeof(fsinfo_t))->addi(&in, oflag);
    PF->init(&out);

    int res = ipc_call(get_vfsd_pid(), VFS_OPEN, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        res = proto_read_int(&out);
        //klog("vfs_open: %s, fd: %d, oflag: %d, mount_pid: %d\n", info->name, res, oflag, info->mount_pid);
        if(res >= 0) {
            proto_read_to(&out, info, sizeof(fsinfo_t));
            fsfile_t* file = vfs_set_file(res, info);	
            if(file != NULL) {
                file->flags = oflag;
                if((oflag & O_APPEND) != 0 &&
                        FS_IS_TYPE(info->type, FS_TYPE_FILE))
                    file->offset = info->stat.size;
            }
        }
        else {
            errno = proto_read_int(&out);
        }
    }
    PF->clear(&out);
    return res;	
}

/*
 * Get the shared-memory pipe ring buffer for a file descriptor.
 * Lazily maps the shared memory on first access (handles fork() correctly:
 * the child inherits the fd with shm_id in fsinfo.data but needs to map it).
 */
static inline shm_pipe_t* get_pipe_shm(int fd, fsfile_t* file) {
    if(fd < 0 || fd >= MAX_OPEN_FILE_PER_PROC)
        return NULL;
    if(_pipe_shm[fd] != NULL)
        return _pipe_shm[fd];
    /* Try to map from fsinfo.data (stores shm_id) */
    int32_t shm_id = (int32_t)file->info.data;
    if(shm_id <= 0)
        return NULL;
    shm_pipe_t* ring = (shm_pipe_t*)shmat(shm_id, NULL, 0);
    if(ring == (void*)-1)
        return NULL;
    _pipe_shm[fd] = ring;
    return ring;
}

static inline uint32_t pipe_live_poll_events(shm_pipe_t* ring) {
    uint32_t events = 0;

    if(ring == NULL)
        return 0;
    if(shm_pipe_readable(ring) > 0)
        events |= VFS_EVT_RD;
    if(shm_pipe_writable(ring) > 0)
        events |= VFS_EVT_WR;
    if(__atomic_load_n(&ring->writer_closed, __ATOMIC_ACQUIRE) ||
            __atomic_load_n(&ring->reader_closed, __ATOMIC_ACQUIRE))
        events |= VFS_EVT_CLOSE;
    return events;
}

/*
 * Tell piped that this fd just moved the ring across the empty/full edge.
 * Skipped entirely while nobody is registered for poll wakeups — a plain
 * blocking reader/writer pair then stays at zero IPC for its whole life.
 * Fire-and-forget: piped snapshots its waiters and wakes them with
 * proc_wakeup_by(pid, ring->node_id).
 */
static void pipe_edge_notify(int fd, fsfile_t* file, shm_pipe_t* ring, uint32_t events) {
    if(__atomic_load_n(&ring->has_poll_waiters, __ATOMIC_ACQUIRE) <= 0)
        return;
    proto_t in;
    PF->init(&in)->addi(&in, (int)events);
    dev_fcntl(file->info.mount_pid, fd, &file->info, PIPE_EDGE, &in, NULL);
    PF->clear(&in);
}

/* Register/unregister this fd as a poll waiter inside piped. On success the
 * caller must sleep on the pipe token (ring->node_id): that is what piped's
 * PIPE_EDGE/close-edge wakes are fired against. */
static int pipe_poll_register(int fd, fsfile_t* file, uint32_t events) {
    proto_t in;
    PF->init(&in)->addi(&in, (int)events);
    int res = dev_fcntl(file->info.mount_pid, fd, &file->info, PIPE_POLL_WAIT, &in, NULL);
    PF->clear(&in);
    return res;
}

static int pipe_poll_unregister(int fd, fsfile_t* file) {
    if(file == NULL)
        return -1;
    return dev_fcntl(file->info.mount_pid, fd, &file->info, PIPE_POLL_UNWAIT, NULL, NULL);
}

static int read_pipe(int fd, void* buf, uint32_t size, bool block) {
    fsfile_t* file = &_fsfiles[fd];
    shm_pipe_t* ring = get_pipe_shm(fd, file);

    if(ring == NULL) {
        /* piped pipes are shm-only: no ring means the pipe id on this fd is
         * broken (or /dev/pipe0 was never wired). There is no vfsd fallback. */
        errno = EIO;
        return -1;
    }

    /*
     * Shared-memory pipe — zero IPC data path.
     *
     * reader_pid/writer_pid are NOT a persistent "who last touched this
     * pipe" marker — they are a one-shot block registration. A peer is
     * stamped ONLY right before it enters proc_block_by, and the counter-
     * party CLEARS the stamp (atomic exchange) after it fires the wake.
     *
     * Why this matters: proc_wakeup_by() in the kernel destroys any non-
     * BLOCK state (WAIT/SLEEP/READY) unconditionally on the else branch.
     * If we kept a stale writer_pid stamped after every write, a later
     * unrelated read would wake a writer that is no longer blocked — e.g.
     * a shell that just fork()+waitpid()'d after echoing its prompt would
     * still be stamped as writer_pid; the telnet relay's next read of
     * pending echo bytes would then yank the shell out of WAIT before its
     * child actually exited, producing a premature prompt.
     *
     * Block/wake token: the pipe's ring->node_id (the read end's node).
     * The two ends are separate anonymous nodes under piped, so sleeping
     * on this fd's own node would mismatch the peer's wakes.
     */
    int32_t my_pid = thread_get_id();
    ewokos_addr_t token = (ewokos_addr_t)ring->node_id;

    while(1) {
        int32_t was_writable = shm_pipe_writable(ring);
        int32_t n = shm_pipe_read(ring, buf, (int32_t)size);
        if(n > 0) {
            /*
             * Shared-memory reads bypass every control plane, so publish the
             * full->not-full edge to piped's poll waiters (only while some
             * exist) and wake the directly-blocked writer through its stamp.
             */
            if(was_writable <= 0 && shm_pipe_writable(ring) > 0)
                pipe_edge_notify(fd, file, ring, VFS_EVT_WR);
            /* Consume the writer's block registration (if any) and wake
             * it exactly once. Exchange guarantees the stamp cannot
             * survive to fire a second, stale wake. */
            int32_t wpid = __atomic_exchange_n(&ring->writer_pid, 0,
                    __ATOMIC_ACQUIRE);
            if(wpid > 0)
                proc_wakeup_by(wpid, token);
            return n;
        }
        /* Ring empty — check if writer closed */
        if(__atomic_load_n(&ring->writer_closed, __ATOMIC_ACQUIRE))
            return -1;
        if(!block)
            return 0;
        /* Register-then-recheck: publish our pid so a writer producing
         * data between the last shm_pipe_read and proc_block_by can
         * still wake us; the kernel latches wake_pending in that gap. */
        __atomic_store_n(&ring->reader_pid, my_pid, __ATOMIC_RELEASE);
        if(shm_pipe_readable(ring) > 0 ||
                __atomic_load_n(&ring->writer_closed, __ATOMIC_ACQUIRE)) {
            /* Producer beat us — retire the registration and retry. */
            __atomic_store_n(&ring->reader_pid, 0, __ATOMIC_RELAXED);
            continue;
        }
        proc_block_by(token);
        __atomic_store_n(&ring->reader_pid, 0, __ATOMIC_RELAXED);
    }
}

static int write_pipe(int fd, const void* buf, uint32_t size, bool block) {
    fsfile_t* file = &_fsfiles[fd];
    shm_pipe_t* ring = get_pipe_shm(fd, file);

    if(ring == NULL) {
        errno = EIO; /* see read_pipe(): shm-only, no vfsd fallback */
        return -1;
    }

    /* See read_pipe() for the stamp/wake protocol and the ring->node_id
     * block token. */
    int32_t my_pid = thread_get_id();
    ewokos_addr_t token = (ewokos_addr_t)ring->node_id;
    const char* p = (const char*)buf;
    int32_t total = 0;

    while(total < (int32_t)size) {
        /* Check if reader closed */
        if(__atomic_load_n(&ring->reader_closed, __ATOMIC_ACQUIRE))
            return total > 0 ? total : -1;

        int32_t was_readable = shm_pipe_readable(ring);
        int32_t n = shm_pipe_write(ring, p + total, (int32_t)size - total);
        if(n > 0) {
            total += n;
            /*
             * Mirror read_pipe(): surface the empty->non-empty edge to
             * piped's poll waiters so poll(POLLIN) readers wake.
             */
            if(was_readable <= 0 && shm_pipe_readable(ring) > 0)
                pipe_edge_notify(fd, file, ring, VFS_EVT_RD);
            /* Consume the reader's block registration (if any) — see
             * read_pipe() for why this must be exchange, not load. */
            int32_t rpid = __atomic_exchange_n(&ring->reader_pid, 0,
                    __ATOMIC_ACQUIRE);
            if(rpid > 0)
                proc_wakeup_by(rpid, token);
        } else {
            /* Ring full */
            if(!block)
                return total > 0 ? total : 0;
            /* Register-then-recheck (mirror of read_pipe). */
            __atomic_store_n(&ring->writer_pid, my_pid, __ATOMIC_RELEASE);
            if(shm_pipe_writable(ring) > 0 ||
                    __atomic_load_n(&ring->reader_closed, __ATOMIC_ACQUIRE)) {
                __atomic_store_n(&ring->writer_pid, 0, __ATOMIC_RELAXED);
                continue;
            }
            proc_block_by(token);
            __atomic_store_n(&ring->writer_pid, 0, __ATOMIC_RELAXED);
        }
    }
    return total;
}

int vfs_close_info(int fd) {
    proto_t in;
    PF->init(&in)->addi(&in, fd);
    int res = ipc_call(get_vfsd_pid(), VFS_CLOSE, &in, NULL);
    PF->clear(&in);
    return res;
}

int vfs_close(int fd) {
    fsfile_t* file = vfs_get_file(fd);
    if(file == NULL)
        return -1;
    if((file->info.state & FS_STATE_CHANGED) != 0) {
        if(vfs_update(&file->info, true) != 0)
            return -1;
    }

    int close_pid = getpid();
    int owner_pid = proc_getpid_or_raw(close_pid);
    proto_t in;
    PF->format(&in, "i,i,m,i,i",
        (ewokos_addr_t)fd, file->info.node, &file->info, sizeof(fsinfo_t),
        (ewokos_addr_t)close_pid, (ewokos_addr_t)owner_pid);
    int res = ipc_call(file->info.mount_pid, FS_CMD_CLOSE, &in, NULL);	
    PF->clear(&in);

    vfs_clear_file(fd);	
    return vfs_close_info(fd);
}

int vfs_dup(int fd) {
    fsfile_t* file = vfs_get_file(fd);
    if(file == NULL)
        return -1;

    proto_t in, out;
    PF->init(&in)->addi(&in, fd);
    PF->init(&out);

    int res = ipc_call(get_vfsd_pid(), VFS_DUP, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        res = proto_read_int(&out);
        if(res >= 0) {
            fsinfo_t info;
            proto_read_to(&out, &info, sizeof(fsinfo_t));
            fsfile_t* file_to = vfs_set_file(res, &info);
            file_to->flags = file->flags;
            /* Propagate shm pipe mapping on dup */
            if(fd >= 0 && fd < MAX_OPEN_FILE_PER_PROC &&
               res >= 0 && res < MAX_OPEN_FILE_PER_PROC)
                _pipe_shm[res] = _pipe_shm[fd];
        }
    }
    PF->clear(&out);
    return res;
}

int vfs_dup2(int fd, int to) {
    fsfile_t* file = vfs_get_file(fd);
    if(file == NULL)
        return -1;
    vfs_clear_file(to);	

    proto_t in, out;
    PF->format(&in, "i,i", (ewokos_addr_t)fd, (ewokos_addr_t)to);
    PF->init(&out);

    int res = ipc_call(get_vfsd_pid(), VFS_DUP2, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        res = proto_read_int(&out);
        if(res >= 0) {
            fsinfo_t info;
            proto_read_to(&out, &info, sizeof(fsinfo_t));
            fsfile_t* file_to = vfs_set_file(res, &info);
            file_to->flags = file->flags;
            /* Propagate shm pipe mapping on dup */
            if(fd >= 0 && fd < MAX_OPEN_FILE_PER_PROC &&
               res >= 0 && res < MAX_OPEN_FILE_PER_PROC)
                _pipe_shm[res] = _pipe_shm[fd];
        }
    }
    PF->clear(&out);
    return res;
}

int vfs_open_pipe(int fd[2]) {
    /*
     * Anonymous pipes live in the standalone pipe driver (piped) mounted at
     * PIPE_DEV_PATH:
     *   fd[0] = open(O_RDONLY)          -> piped creates pipe + shm ring
     *   fd[1] = open(O_WRONLY)          -> unattached write end
     *   fcntl(PIPE_ATTACH, shm_id)      -> bind write end to the pipe
     * The open reply carries FS_TYPE_PIPE + the shm id in fsinfo.data, so the
     * fast path (and post-fork lazy shmat) works from both ends.
     */
    fsinfo_t info;
    if(vfs_get_by_name(PIPE_DEV_PATH, &info) != 0)
        return -1;
    int rfd = vfs_open(&info, O_RDONLY);
    if(rfd < 0)
        return -1;
    /* let piped create the pipe + shm ring; persist its reply (FS_TYPE_PIPE +
     * shm id in fsinfo.data) on the fd side, exactly like _open() does */
    if(dev_open(info.mount_pid, rfd, &info, O_RDONLY) != 0 ||
            vfs_set_by_fd(rfd, &info) != 0 ||
            !FS_IS_TYPE(info.type, FS_TYPE_PIPE) || (int32_t)info.data <= 0) {
        /* PIPE_DEV_PATH exists but is not piped (machine not wired) */
        vfs_close(rfd);
        errno = ENOENT;
        return -1;
    }

    /* fresh lookup: the read-end open left 'info' pointing at that end's
     * anonymous node; opening again must create a second node */
    fsinfo_t winfo;
    if(vfs_get_by_name(PIPE_DEV_PATH, &winfo) != 0) {
        vfs_close(rfd);
        return -1;
    }
    int wfd = vfs_open(&winfo, O_WRONLY);
    if(wfd < 0) {
        vfs_close(rfd);
        return -1;
    }
    if(dev_open(winfo.mount_pid, wfd, &winfo, O_WRONLY) != 0 ||
            vfs_set_by_fd(wfd, &winfo) != 0) {
        vfs_close(wfd);
        vfs_close(rfd);
        return -1;
    }

    proto_t in, out;
    PF->init(&in)->addi(&in, (int)info.data);
    PF->init(&out);
    int res = dev_fcntl(winfo.mount_pid, wfd, &winfo, PIPE_ATTACH, &in, &out);
    PF->clear(&in);
    PF->clear(&out);
    if(res != 0) {
        vfs_close(wfd);
        vfs_close(rfd);
        return -1;
    }
    /* piped answered with fsinfo.data = shm id; persist on the fd side so
     * post-fork lazy shmat() works from the write end too */
    winfo.data = info.data;
    vfs_set_by_fd(wfd, &winfo);

    fsfile_t* read_end = vfs_get_file(rfd);
    fsfile_t* write_end = vfs_get_file(wfd);
    if(read_end != NULL) {
        read_end->flags = O_RDONLY;
        read_end->fd_flags = FD_CLOEXEC;
    }
    if(write_end != NULL) {
        write_end->flags = O_WRONLY;
        write_end->fd_flags = FD_CLOEXEC;
        /* pipe id for lazy shmat() after fork (dev_fcntl already refreshed
         * winfo.data; keep the local copy authoritative) */
        write_end->info.data = info.data;
    }

    /* Map the shared-memory ring once for both ends */
    int32_t shm_id = (int32_t)info.data;
    shm_pipe_t* ring = (shm_pipe_t*)shmat(shm_id, NULL, 0);
    if(ring != (void*)-1) {
        if(rfd >= 0 && rfd < MAX_OPEN_FILE_PER_PROC)
            _pipe_shm[rfd] = ring;
        if(wfd >= 0 && wfd < MAX_OPEN_FILE_PER_PROC)
            _pipe_shm[wfd] = ring;
    }

    fd[0] = rfd;
    fd[1] = wfd;
    return 0;
}

int vfs_get_mount_by_id(int id, mount_t* mount) {
    proto_t in, out;
    PF->init(&in)->addi(&in, id);
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_GET_MOUNT_BY_ID, &in, &out);
    PF->clear(&in);

    if(res == 0) {
        res = proto_read_int(&out);
        if(res == 0)
            proto_read_to(&out, mount, sizeof(mount_t));
    }
    PF->clear(&out);
    return res;
}

int vfs_get_mount_by_fname(const char* fname, mount_t* mount, char* dev_fname, int dev_fname_sz) {
    int max_len = 0;
    for(int i=0; i<FS_MOUNT_MAX; i++) {
        mount_t mnt;
        if(vfs_get_mount_by_id(i, &mnt) != 0)
            continue;
        const char* p = strstr(fname, mnt.org_name);
        if(p != NULL) {
            if(strlen(mnt.org_name) > max_len) {
                max_len = strlen(mnt.org_name);
                memset(dev_fname, 0, dev_fname_sz);
                memcpy(dev_fname, p + strlen(mnt.org_name) - 1, dev_fname_sz-1);
                if(dev_fname[0] == 0)
                    dev_fname[0] = '//';
                memcpy(mount, &mnt, sizeof(mount_t));
            }
        }
    }
    if(max_len > 0)
        return 0;

    memset(mount, 0, sizeof(mount_t));
    return -1;
}

int vfs_get_by_name(const char* fname, fsinfo_t* info) {
    int res;
    char fullname[FS_FULL_NAME_MAX+1] = {0};
    char dev_fname[FS_FULL_NAME_MAX+1] = {0};
    vfs_fullname(fname, fullname, FS_FULL_NAME_MAX);

    /* //read from mounted dev
    mount_t mnt;
    res = vfs_get_mount_by_fname(fullname, &mnt, dev_fname, FS_FULL_NAME_MAX);
    if(res == 0) {
        res = dev_get_by_name(mnt.pid, dev_fname, info);
        if(res == 0) {
            info->mount_pid = mnt.pid;
            klog("mnt: %d, fs_get_by_name: %s, mnt_point: %s, dev_fname: %s\n",
                    info->mount_pid, fullname, mnt.org_name, dev_fname);
            return 0;
        }
    }
        */

    proto_t in, out;
    PF->init(&in)->adds(&in, fullname);
    PF->init(&out);
    res = ipc_call(get_vfsd_pid(), VFS_GET_BY_NAME, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        res = proto_read_int(&out); //res = node
        if(res != 0) {
            if(info != NULL){
                proto_read_to(&out, info, sizeof(fsinfo_t));
                //fix me: update stat form device
                dev_stat(info->mount_pid, info, &info->stat);
            }
            res = 0;
        }
        else
            res = -1;
    }
    PF->clear(&out);
    return res;	
}

fsinfo_t* vfs_kids(fsinfo_t* info, uint32_t *num) {
    fsinfo_t* ret = NULL;

    /* //read from mounted dev
    if(info->mount_pid > 0)
        ret = dev_kids(info->mount_pid, info, num);
    if(ret != NULL) 
        return ret;
        */

    proto_t in, out;
    PF->init(&in)->addi(&in, info->node);
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_GET_KIDS, &in, &out);
    PF->clear(&in);

    if(res == 0) {
        uint32_t n = proto_read_int(&out);
        *num = n;
        if(n > 0) {
            ret = (fsinfo_t*)malloc(n * sizeof(fsinfo_t));
            proto_read_to(&out, ret, n * sizeof(fsinfo_t));
        }
    }
    PF->clear(&out);
    return ret;
}

int vfs_update(fsinfo_t* info, bool do_dev) {
    if(do_dev) {
        if(dev_set(info->mount_pid, info) != 0)
            return -1;
    }

    if(vfs_set_info(info) != 0)
        return -1;
    vfs_update_file(info);
    return 0;
}

int vfs_del_node(ewokos_addr_t node) {
    proto_t in, out;
    PF->init(&in)->addi(&in, node);
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_DEL_NODE, &in, &out);
    PF->clear(&in);

    if(res == 0) {
        res = proto_read_int(&out);
    }
    PF->clear(&out);
    return res;
}

/*
int vfs_get_mount(fsinfo_t* info, mount_t* mount) {
    proto_t in, out;
    PF->init(&in)->add(&in, info, sizeof(fsinfo_t));
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_GET_MOUNT, &in, &out);
    PF->clear(&in);

    if(res == 0) {
        proto_read_to(&out, mount, sizeof(mount_t));
    }
    PF->clear(&out);
    return res;
}
*/


int vfs_mount(ewokos_addr_t mount_node_to, ewokos_addr_t node, const char* dev_desc) {
    proto_t in, out;
    PF->init(&in)->
        addi(&in, mount_node_to)->
        addi(&in, node)->
        adds(&in, dev_desc);
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_MOUNT, &in, &out);
    PF->clear(&in);

    if(res == 0) {
        res = proto_read_int(&out);
    }
    PF->clear(&out);
    return res;
}

int vfs_umount(ewokos_addr_t node) {
    proto_t in, out;
    PF->init(&in)->addi(&in, node);
    PF->init(&out);
    int res = ipc_call(get_vfsd_pid(), VFS_UMOUNT, &in, &out);
    PF->clear(&in);

    if(res == 0) {
        res = proto_read_int(&out);
    }
    PF->clear(&out);
    return res;
}

int vfs_tell(int fd) {
    fsfile_t* buffer = vfs_get_file(fd);
    if(buffer == NULL)
        return 0;
    return buffer->offset;
}

int vfs_seek(int fd, int offset) {
    fsfile_t* buffer = vfs_get_file(fd);
    if(buffer == NULL)
        return -1;
    
    buffer->offset = offset;
    return 0;
}

#define VFS_BUF_SIZE (1024*256)

uint8_t* vfs_readfile(const char* fname, int* rsz) {
    char fullname[FS_FULL_NAME_MAX+1] = {0};
    vfs_fullname(fname, fullname, FS_FULL_NAME_MAX);
    fsinfo_t info;
    if(vfs_get_by_name(fullname, &info) != 0 || info.stat.size <= 0)
        return NULL;
    uint8_t* buf = (uint8_t*)malloc(info.stat.size+1); //one more char for string end.
    if(buf == NULL)
        return NULL;

    char* p = (char*)buf;
    int fd = vfs_open(&info, O_RDONLY);
    int fsize = info.stat.size;
    if(fd >= 0) {
        while(fsize > 0) {
            int sz = read(fd, p, VFS_BUF_SIZE < fsize ? VFS_BUF_SIZE:fsize);
            if(sz <= 0 && errno != EAGAIN)
                break;
            if(sz > 0) {
                fsize -= sz;
                p += sz;
            }
            else {
                /* fd became temporarily empty (VFS_ERR_RETRY): park on the
                 * node's RD wait queue with a bounded deadline instead of
                 * the old 1ms usleep probe loop; the driver's wake edge
                 * releases us early. */
                vfs_block_by_fd_timeout(fd, VFS_EVT_RD, 100*1000);
            }
        }
        close(fd);
    }

    if(fsize != 0) {
        free(buf);
        return NULL;
    }

    if(rsz != NULL)
        *rsz = info.stat.size;
    buf[info.stat.size] = 0;
    return buf;
}

int vfs_create_uid(const char* fname, fsinfo_t* ret, int type, int mode, bool vfs_node_only, bool autodir, int uid, int gid) {
    char dir[FS_FULL_NAME_MAX];
    char name[FS_FULL_NAME_MAX];
    vfs_dir_name(fname, dir, FS_FULL_NAME_MAX);
    vfs_file_name(fname, name, FS_FULL_NAME_MAX);

    fsinfo_t info_to;
    if(vfs_get_by_name(dir, &info_to) != 0) {
        int res_dir = -1;
        if(autodir)
            res_dir = vfs_create_uid(dir, &info_to, FS_TYPE_DIR, 0755, vfs_node_only, autodir, uid, gid);
        if(res_dir != 0)
            return -1;
    }

    if(name == NULL || name[0] == 0)
        return 0;

    fsinfo_t fi;
    memset(&fi, 0, sizeof(fsinfo_t));
    strcpy(fi.name, name);
    fi.type = type;
    if(FS_IS_TYPE(type, FS_TYPE_DIR)) {
        fi.stat.size = 1024;
        fi.state |= FS_STATE_KIDS_LOADED;
    }

    fi.stat.uid = uid < 0 ? getuid() : uid;
    fi.stat.gid = gid < 0 ? getgid() : gid;
    fi.stat.mode = mode;

    if(vfs_new_node(&fi, info_to.node, vfs_node_only, false) != 0)
        return -1;

    if(vfs_node_only) {//only create in vfs service, not existed in storage. 
        if(ret != NULL)
            memcpy(ret, &fi, sizeof(fsinfo_t));
        return 0;
    }

    fi.mount_pid = info_to.mount_pid;
    int res = dev_create(info_to.mount_pid, &info_to, &fi);
    if(res == 0) 
        vfs_set_info(&fi);

    if(res != 0) {
        vfs_del_node(fi.node);
        return res;
    }

    if(ret != NULL)
        memcpy(ret, &fi, sizeof(fsinfo_t));
    return res;
}

int vfs_create(const char* fname, fsinfo_t* ret, int type, int mode, bool vfs_node_only, bool autodir) {
    return vfs_create_uid(fname, ret, type, mode, vfs_node_only, autodir, -1, -1);
}

const char* vfs_dir_name(const char* fname, char* ret, uint32_t len) {
    static char ret_buf[FS_FULL_NAME_MAX];
    if(ret == NULL) {
        ret = ret_buf;
        len = FS_FULL_NAME_MAX;
    }
    memset(ret, 0, len);

    if(fname == NULL)
        return ret;

    strncpy(ret, fname, len-1);
    int i = strlen(ret)-1;
    while(i >= 0) {
        if(ret[i] == '/') {
            ret[i] = 0;
            break;
        }
        --i;	
    }

    if(ret[0] == 0)
        strcpy(ret, "/");
    return ret;
}

const char* vfs_file_name(const char* fname, char* ret, uint32_t len) {
    static char ret_buf[FS_FULL_NAME_MAX];
    if(ret == NULL) {
        ret = ret_buf;
        len = FS_FULL_NAME_MAX;
    }
    memset(ret, 0, len);

    if(fname == NULL)
        return ret;

    int i = strlen(fname)-1;
    while(i >= 0) {
        if(fname[i] == '/') {
            break;
        }
        --i;	
    }
    ++i;
    strncpy(ret, fname+i, len-1);
    return ret;
}

const char* vfs_full_file_name(const char* fname, const char* ref_fname, char* ret, uint32_t len) {
    static char ret_buf[FS_FULL_NAME_MAX];
    if(ret == NULL) {
        ret = ret_buf;
        len = FS_FULL_NAME_MAX;
    }
    memset(ret, 0, len);

    if(fname == NULL)
        return ret;

    if(fname[0] == '/') {
        strncpy(ret, fname, len-1);
        return ret;
    }

    char dir_name[FS_FULL_NAME_MAX] = {0};
    vfs_dir_name(ref_fname, dir_name, FS_FULL_NAME_MAX);
    snprintf(ret, len-1, "%s/%s", dir_name, fname);
    return ret;
}

int vfs_fcntl(int fd, int cmd, proto_t* arg_in, proto_t* arg_out) {
    fsinfo_t info;
    if(vfs_get_by_fd(fd, &info) != 0)
        return -1;
    int res = dev_fcntl(info.mount_pid, fd, &info, cmd, arg_in, arg_out);
    if(res == 0)
        vfs_update_file(&info);
    return res;
}

inline int  vfs_fcntl_wait(int fd, int cmd, proto_t* in) {
    proto_t out;
    PF->init(&out);
    int res = vfs_fcntl(fd, cmd, in, &out);
    PF->clear(&out);
    return res;
}

int32_t vfs_shm(int fd, uint8_t* contig, int* size) {
    fsinfo_t info;
    if(vfs_get_by_fd(fd, &info) != 0)
        return 0;
    return dev_shm(info.mount_pid, fd, info.node, contig, size);
}

int vfs_flush(int fd, bool wait) {
    fsinfo_t info;
    if(vfs_get_by_fd(fd, &info) != 0)
        return 0; //error

    return dev_flush(info.mount_pid, fd, info.node, wait);
}

int vfs_read_pipe(int fd, ewokos_addr_t node, void* buf, uint32_t size, bool block) {
    (void)node; /* piped pipes block/wake on the ring token, not this fd's node */
    int res = read_pipe(fd, buf, size, block);
    if(res == 0) { // pipe empty, do retry
        errno = EAGAIN;
        return -1;
    }
    if(res > 0) {
        return res;
    }
    return 0; //res < 0 , pipe closed, return 0.
}

int vfs_read(int fd, fsinfo_t *info, void* buf, uint32_t size) {
    errno = 0;
    int offset = 0;
    if(FS_IS_TYPE(info->type, FS_TYPE_FILE) ||
            FS_IS_TYPE(info->type, FS_TYPE_CHAR)) {
        offset = vfs_tell(fd);
        if(offset < 0)
            offset = 0;
    }

    int res = dev_read(info->mount_pid, fd, info, offset, buf, size);
    if(res > 0) {
        offset += res;
        if(FS_IS_TYPE(info->type, FS_TYPE_FILE) ||
                FS_IS_TYPE(info->type, FS_TYPE_CHAR))
            vfs_seek(fd, offset);
    }
    else if(res == VFS_ERR_RETRY) {
        errno = EAGAIN;
        res = -1;
    }
    return res;
}

int vfs_write_pipe(int fd, ewokos_addr_t node, const void* buf, uint32_t size, bool block) {
    (void)node; /* piped pipes block/wake on the ring token, not this fd's node */
    int res = write_pipe(fd, buf, size, block);
    if(res == 0) { // pipe not empty, do retry
        errno = EAGAIN;
        return -1;
    }
    if(res > 0)
        return res;
    return 0; //res < 0 , pipe closed, return 0.
}

int vfs_write(int fd, fsinfo_t* info, const void* buf, uint32_t size) {
    if(FS_IS_TYPE(info->type, FS_TYPE_DIR)) 
        return -1;

    int offset = 0;
    fsfile_t* file = vfs_get_file(fd);
    if(FS_IS_TYPE(info->type, FS_TYPE_FILE)) {
        if(file != NULL && (file->flags & O_APPEND) != 0) {
            fsinfo_t latest;
            if(vfs_get_by_node(info->node, &latest) == 0) {
                memcpy(info, &latest, sizeof(fsinfo_t));
                memcpy(&file->info, &latest, sizeof(fsinfo_t));
                offset = latest.stat.size;
            }
            else {
                offset = info->stat.size;
            }
        }
        else {
            offset = vfs_tell(fd);
        }
        if(offset < 0)
            offset = 0;
    }
        
    int res = dev_write(info->mount_pid, fd, info, offset, buf, size);
    if(res > 0) {
        offset += res;
        if(FS_IS_TYPE(info->type, FS_TYPE_FILE)) {
            /*
             * Drivers such as ramfs can mutate per-file fsinfo.data/stat.size on write.
             * Sync the updated fsinfo back to vfsd's node so reopening the same path
             * (for shell pipe temp files, redirections, etc.) sees the new backing data.
             *
             * For plain sequential growth (only stat fields move) the push is
             * throttled to 256KB boundaries: the server tags the reply fsinfo
             * with FS_STATE_CHANGED, so vfs_close pushes the final state and
             * every intermediate update during e.g. a large file copy is just
             * one wasted IPC round-trip per write.
             */
            bool push = (file == NULL) ||
                    (file->info.data != info->data) ||
                    (((uint32_t)offset >> 18) != ((uint32_t)file->info.stat.size >> 18));
            vfs_update_file(info);
            if(push)
                vfs_set_info(info);
            vfs_seek(fd, offset);
        }
    }
    else if(res == VFS_ERR_RETRY) {
        errno = EAGAIN;
        res = -1;
    }
    return res;
}

static int vfs_block_raw(ewokos_addr_t node, int event) {
    proto_t in;
    PF->init(&in)->
        addi(&in, node)->
        addi(&in, event);
    int res = ipc_call(get_vfsd_pid(), VFS_BLOCK, &in, NULL);
    PF->clear(&in);
    return res;
}

static uint32_t vfs_get_poll_events_by_node(ewokos_addr_t node_id) {
    proto_t in, out;
    PF->init(&in)->
        addi(&in, node_id);
    PF->init(&out);
    ipc_call(get_vfsd_pid(), VFS_GET_POLL_EVENTS, &in, &out);
    PF->clear(&in);
    uint32_t res = (uint32_t)proto_read_int(&out);
    PF->clear(&out);
    return res;
}

int  vfs_block_by_fd(int fd, int event) {
    fsinfo_t info;
    uint32_t wait_events = (uint32_t)event | VFS_EVT_CLOSE | VFS_EVT_ERR | VFS_EVT_NVAL;

    if(vfs_get_by_fd(fd, &info) != 0 || info.node == 0)
        return -1;

    while(1) {
        /*
         * For shared device nodes (notably /dev/net0), readiness is fd-local.
         * Re-check visibility through vfs_get_poll_events(fd) so one socket's
         * stale sticky node bit does not make another socket skip the sleep
         * forever.
         */
        if((vfs_get_poll_events(fd) & wait_events) != 0) {
            vfs_unblock(info.node);
            return 0;
        }
        if(vfs_block_raw(info.node, event) != 0)
            return -1;
        if((vfs_get_poll_events(fd) & wait_events) != 0) {
            vfs_unblock(info.node);
            return 0;
        }
        proc_block_by(info.node);
    }
    return 0;
}

/*
 * Timed variant of vfs_block_by_fd(): wait up to timeout_usec for the fd's
 * event, returning 0 on readiness and -1 on timeout/error.
 *
 * Replaces the old usleep(1000)-and-probe-again retry loops scattered over
 * the EAGAIN paths (printf, shell pipe writes, vfs_readfile): instead of
 * waking 1000 times a second to re-issue the IO probe, the caller registers
 * on the node's vfsd wait queue and parks in proc_block_timeout(), released
 * early by the driver's token-scoped wake edge or at the deadline. Each
 * round re-checks the per-fd live visibility and re-registers, mirroring
 * vfs_block_by_fd(); the registration is dropped before returning so no
 * stale waiter survives.
 */
int  vfs_block_by_fd_timeout(int fd, int event, uint32_t timeout_usec) {
    fsinfo_t info;
    uint32_t wait_events = (uint32_t)event | VFS_EVT_CLOSE | VFS_EVT_ERR | VFS_EVT_NVAL;

    if(vfs_get_by_fd(fd, &info) != 0 || info.node == 0)
        return -1;

    uint64_t start_ms = kernel_tic_ms(0);
    bool registered = false;
    while(1) {
        if((vfs_get_poll_events(fd) & wait_events) != 0) {
            if(registered)
                vfs_unblock(info.node);
            return 0;
        }
        uint64_t elapsed_ms = kernel_tic_ms(0) - start_ms;
        if(elapsed_ms * 1000ULL >= (uint64_t)timeout_usec)
            break;
        uint64_t remain_us = (uint64_t)timeout_usec - elapsed_ms * 1000ULL;
        if(!registered && vfs_block_raw(info.node, event) != 0)
            return -1;
        registered = true;
        if((vfs_get_poll_events(fd) & wait_events) != 0) {
            vfs_unblock(info.node);
            return 0;
        }
        /*
         * Timed, token-scoped sleep: proc_block_timeout() discards the
         * latched generic token-0 wakes our vfs_block_raw() IPC left behind,
         * so the wait stays parked until a real edge for this node (or the
         * deadline) arrives.
         */
        uint32_t slice = (remain_us > 0xffffffffULL) ? 0xffffffffU : (uint32_t)remain_us;
        proc_block_timeout(info.node, slice);
    }
    if(registered)
        vfs_unblock(info.node);
    return -1;
}

int  vfs_block(ewokos_addr_t node, int event) {
    uint32_t wait_events = (uint32_t)event | VFS_EVT_CLOSE | VFS_EVT_ERR | VFS_EVT_NVAL;
    while(1) {
        /*
         * Check the sticky node event before asking vfsd to enqueue us. If the
         * edge is already visible, returning immediately avoids the race where
         * VFS_BLOCK sees the event and skips queueing us, another path consumes
         * the event, and we then go to sleep without ever being on the wait
         * queue for the next wakeup.
         */
        if((vfs_get_poll_events_by_node(node) & wait_events) != 0) {
            vfs_unblock(node);
            return 0;
        }
        if(vfs_block_raw(node, event) != 0)
            return -1;
        if((vfs_get_poll_events_by_node(node) & wait_events) != 0) {
            vfs_unblock(node);
            return 0;
        }
        /*
         * Block scoped to this node id so only a wakeup for THIS node (or a
         * generic tokenless wake) can release us. A tokenless proc_block()
         * here would let an unrelated node's edge (e.g. tty keyboard RD)
         * spuriously wake this block via the kernel's sticky wake latch.
         */
        proc_block_by(node);
    }
    return 0;
}

int  vfs_unblock(ewokos_addr_t node) {
    proto_t in;
    PF->init(&in)->
        addi(&in, node);
    ipc_call(get_vfsd_pid(), VFS_UNBLOCK, &in, NULL);
    PF->clear(&in);
    return 0;
}

int  vfs_wakeup(ewokos_addr_t node, int events) {
    proto_t in;
    PF->init(&in)->
        addi(&in, node)->
        addi(&in, events);

    ipc_call(get_vfsd_pid(), VFS_WAKEUP, &in, NULL);
    PF->clear(&in);
    return 0;
}

int  vfs_clear_poll_events(ewokos_addr_t node_id, uint32_t events) {
    proto_t in, out;
    PF->init(&in)->
        addi(&in, node_id)->
        addi(&in, events);
    PF->init(&out);
    
    int res = ipc_call(get_vfsd_pid(), VFS_CLEAR_POLL_EVENTS, &in, &out);
    PF->clear(&in);
    if(res == 0)
        res = proto_read_int(&out);
    PF->clear(&out);
    return res;
}

uint32_t  vfs_get_poll_events(int fd) {
    fsinfo_t info;
    if(vfs_get_by_fd(fd, &info) != 0 || info.node == 0)
        return 0;

    if(FS_IS_TYPE(info.type, FS_TYPE_PIPE)) {
        fsfile_t* file = vfs_get_file(fd);
        shm_pipe_t* ring = NULL;
        if(file != NULL)
            ring = get_pipe_shm(fd, file);
        /* piped pipes are fully described by the ring's live state: vfsd's
         * sticky node bits are never published for them, so skip that IPC. */
        if(ring != NULL)
            return pipe_live_poll_events(ring);
    }

    uint32_t sticky = vfs_get_poll_events_by_node(info.node);
    uint32_t live = 0;
    if(info.mount_pid > 0 && dev_poll(info.mount_pid, fd, &info, &live) == 0) {
        uint32_t live_rw = live & VFS_EVT_RW;
        /*
         * Device nodes such as /dev/net0 can multiplex many logical
         * sockets behind one shared VFS node. Their RD/WR readiness is
         * fd-local, not node-global, so clearing sticky RW here based on
         * THIS fd's live state can erase another socket's only wake edge
         * before it blocks for accept()/send()/recv() again. Keep using
         * the live per-fd readiness to filter visibility, but leave the
         * shared node's sticky RW bits alone. Shared-node sleepers now
         * use vfs_block_by_fd() so they can wait on per-fd visibility
         * without globally consuming a sibling socket's edge.
         */
        return (sticky & ~VFS_EVT_RW) | live_rw;
    }
    return sticky;
}

static uint32_t vfs_get_poll_events_cached(int fd, const fsinfo_t* info) {
    if(info == NULL || info->node == 0)
        return 0;

    if(FS_IS_TYPE(info->type, FS_TYPE_PIPE)) {
        fsfile_t* file = vfs_get_file(fd);
        shm_pipe_t* ring = NULL;
        if(file != NULL)
            ring = get_pipe_shm(fd, file);
        /* same as vfs_get_poll_events(): the ring's live state is complete
         * for piped pipes, no vfsd sticky-bits IPC needed */
        if(ring != NULL)
            return pipe_live_poll_events(ring);
    }

    uint32_t sticky = vfs_get_poll_events_by_node(info->node);
    uint32_t live = 0;
    fsinfo_t live_info = *info;
    if(live_info.mount_pid > 0 && dev_poll(live_info.mount_pid, fd, &live_info, &live) == 0) {
        uint32_t live_rw = live & VFS_EVT_RW;
        /*
         * Same shared-device rule as vfs_get_poll_events(): do not let
         * one fd's live readiness clear RW bits for sibling sockets that
         * share the same node id (notably /dev/net0).
         */
        return (sticky & ~VFS_EVT_RW) | live_rw;
    }
    return sticky;
}

int vfs_poll(vfs_pollfd_t* fds, int num, int timeout) {
    if(num <= 0)
        return -1;

    int res = 0;
    poll_fd_cache_t* cache = NULL;
    bool registered = false;
    uint32_t backoff_us = VFS_POLL_BACKOFF_START_US;
    uint64_t start_ms = 0;
    if(timeout > 0)
        start_ms = kernel_tic_ms(0);

    cache = (poll_fd_cache_t*)calloc((size_t)num, sizeof(poll_fd_cache_t));
    if(cache == NULL)
        return -1;

    while(true) {
        for(int i = 0; i < num; ++i) {
            cache[i].valid = false;
            cache[i].pipe_wait = false;
            memset(&cache[i].info, 0, sizeof(fsinfo_t));
            if(vfs_get_by_fd(fds[i].fd, &cache[i].info) == 0 &&
                    cache[i].info.node != 0) {
                cache[i].valid = true;
            }
        }

        /* Phase 1: Check all FDs for current events */
        res = 0;
        for(int i = 0; i < num; ++i) {
            uint32_t visible;
            uint32_t mask = (uint32_t)fds[i].events | VFS_EVT_CLOSE | VFS_EVT_ERR | VFS_EVT_NVAL;

            if(cache[i].valid)
                visible = vfs_get_poll_events_cached(fds[i].fd, &cache[i].info);
            else
                visible = 0;
            fds[i].revents = (uint16_t)(visible & mask);
            if(fds[i].revents != 0)
                res++;
        }
        if(res > 0)
            break;

        /* Phase 2: Check timeout */
        if(timeout == 0)
            break;
        if(timeout > 0) {
            uint64_t now_ms = kernel_tic_ms(0);
            if((now_ms - start_ms) >= (uint64_t)timeout)
                break;
        }

        /*
         * Phase 3: Register a waiter for every valid fd.
         *
         * shm pipe fds park in piped's waiter table (one slot per pipe per
         * pid, so several pipes of one process never overwrite each other);
         * everything else registers on its vfsd node. vfsd keeps one read
         * and one write waiter per pid, so with several device fds the LAST
         * registration wins there — events on the evicted nodes never wake
         * the block, which is why even an infinite-timeout block below
         * carries a bounded deadline and every poll round re-registers.
         * Coverage stays correct without needing per-fd waiter slots in
         * vfsd.
         */
        uint32_t wait_node = 0;
        for(int i = 0; i < num; ++i) {
            if(!cache[i].valid)
                continue;
            fsfile_t* file = vfs_get_file(fds[i].fd);
            shm_pipe_t* ring = NULL;
            if(FS_IS_TYPE(cache[i].info.type, FS_TYPE_PIPE) && file != NULL)
                ring = get_pipe_shm(fds[i].fd, file);
            if(ring != NULL) {
                /*
                 * piped wakes poll waiters with proc_wakeup_by(pid,
                 * ring->node_id); remember one pipe token as the scoped
                 * block target below.
                 */
                if(pipe_poll_register(fds[i].fd, file, (uint32_t)fds[i].events) == 0) {
                    cache[i].pipe_wait = true;
                    wait_node = (uint32_t)ring->node_id;
                    continue;
                }
            }
            wait_node = cache[i].info.node;
            vfs_block_raw(cache[i].info.node, (int)fds[i].events);
        }
        registered = true;

        /*
         * Phase 4: Re-check after registration (prevent race).
         *
         * This must use the full visible readiness, not only the sticky node
         * bits. Socket/device drivers can publish a live RD/WR state from
         * dev_poll() before a new vfs_wakeup() edge is latched. Looking only
         * at sticky events here misses that window and the subsequent block
         * can sleep until the timeout even though the fd is already ready.
         */
        bool ready_during_register = false;
        for(int i = 0; i < num; ++i) {
            if(cache[i].valid) {
                uint32_t visible = vfs_get_poll_events_cached(fds[i].fd,
                        &cache[i].info);
                uint32_t mask = (uint32_t)fds[i].events | VFS_EVT_CLOSE | VFS_EVT_ERR | VFS_EVT_NVAL;
                if((visible & mask) != 0) {
                    ready_during_register = true;
                    break;
                }
            }
        }
        if(ready_during_register)
            continue; /* Events appeared during registration, re-do full check */

        /*
         * Phase 5: Really sleep until a registered source wakes us.
         *
         * Single fd: block scoped to that node token (vfsd wakes registered
         * poll waiters with proc_wakeup_by(pid, node); piped uses the pipe
         * ring token). Multiple fds: a generic block, released by ANY node
         * token wake — a wake from any registered source carries a non-zero
         * token. But vfsd armed only the LAST registered node (one read
         * waiter per pid), so fds evicted from the waiter slot never raise
         * a wake: with an infinite timeout the block below must stay bounded
         * by the capped backoff, otherwise input on an evicted fd (telnet's
         * keyboard, evicted by its socket registration) is lost forever.
         * proc_block_timeout discards the latched generic token-0 wakes our
         * registration IPCs left behind and honours the deadline, so a
         * finite timeout no longer degrades into a usleep/probe spin loop:
         * an idle waiter issues zero readiness IPCs until an event edge or
         * its deadline arrives.
         */
        if(num == 1) {
            if(timeout < 0) {
                if(wait_node != 0)
                    proc_block_by(wait_node);
                else
                    proc_block();
            }
            else {
                uint64_t elapsed = kernel_tic_ms(0) - start_ms;
                uint64_t remaining_us = ((uint64_t)timeout > elapsed) ?
                        ((uint64_t)timeout - elapsed) * 1000 : 1;
                if(remaining_us > 0xffffffffULL)
                    remaining_us = 0xffffffffULL;
                if(wait_node != 0)
                    proc_block_timeout(wait_node, (uint32_t)remaining_us);
                else
                    usleep((uint32_t)remaining_us);
            }
        }
        else {
            if(timeout < 0) {
                /*
                 * Never block unbounded here: vfsd armed only the LAST
                 * registered node (one read waiter per pid), so events on
                 * the evicted fds never wake us — they are caught by the
                 * level-triggered re-check after this bounded deadline.
                 */
                proc_block_timeout(0, backoff_us);
                if(backoff_us < VFS_MULTI_POLL_BACKOFF_US) {
                    backoff_us *= 2;
                    if(backoff_us > VFS_MULTI_POLL_BACKOFF_US)
                        backoff_us = VFS_MULTI_POLL_BACKOFF_US;
                }
            }
            else {
                uint64_t elapsed = kernel_tic_ms(0) - start_ms;
                uint64_t remaining_us = ((uint64_t)timeout > elapsed) ?
                        ((uint64_t)timeout - elapsed) * 1000 : 1;
                if(remaining_us > 0xffffffffULL)
                    remaining_us = 0xffffffffULL;
                proc_block_timeout(0, (uint32_t)remaining_us);
            }
        }
    }

    /*
     * Drop our registrations from every polled node before returning. Leaving
     * stale waiters behind lets a later event on any of these nodes wake this
     * process while it is blocked on something unrelated, because the kernel
     * proc_wakeup() is not scoped to a node.
     */
    if(registered) {
        for(int i = 0; i < num; ++i) {
            if(cache != NULL && cache[i].valid) {
                if(cache[i].pipe_wait)
                    pipe_poll_unregister(fds[i].fd, vfs_get_file(fds[i].fd));
                else
                    vfs_unblock(cache[i].info.node);
            }
        }
    }
    free(cache);
    return res;
}

#ifdef __cplusplus
}
#endif
