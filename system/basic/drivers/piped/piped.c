/*
 * piped.c - standalone pipe driver.
 *
 * Owns every anonymous pipe created by pipe()/pipe2().
 *
 * MUST run single-task (no IPC_MULTI_TASK): the lifecycle refcount protocol
 * only stays correct when request PROCESSING order matches ACCEPTANCE order.
 * vfsd ships fork/dup2 inheritance FS_CMD_DUPs synchronously (before the new
 * holder can run and close anything) and libc ships FS_CMD_CLOSE directly,
 * so every dup is accepted into piped's kernel queue before any paired close.
 * Single-task serving is a strict FIFO ring (head-only execution, head-only
 * pop), so a close can never be processed before the dup that justifies it.
 * A multi_task worker pool completes requests out of order - and can even
 * drop an accepted task silently via watchdog abort - so a reordered close
 * drives the refcount to zero while descriptors still live, latching
 * writer_closed/reader_closed: readers then see instant EOF (empty
 * "ps|dump") and writers see EPIPE mid-pipeline. All handlers are O(1)
 * registry ops (data moves through the shm ring, never through this
 * process), so concurrent serving would buy nothing anyway.
 *
 * Data transfer never comes here at all — reader and writer move bytes
 * through the shared-memory ring (ewoksys/shm_pipe.h) and block/wake each
 * other with direct proc_block_by()/proc_wakeup_by() on the pipe's block
 * token (ring->node_id). piped only owns:
 *
 *   - lifecycle refcounting: open(O_RDONLY) creates the pipe + ring,
 *     open(O_WRONLY) + PIPE_ATTACH binds the write end (netd SOCK_LINK
 *     pattern); FS_CMD_DUP/FS_CMD_CLOSE mirror vfsd's driver_ref
 *     bookkeeping (+1 per inherited/forked ref-owning fd, -1 per close).
 *   - wakeup delivery for events the one-shot pid stamps cannot express:
 *     poll waiters (PIPE_POLL_WAIT registrations) and close edges. Wakes use
 *     proc_wakeup_by(pid, token) from snapshots taken under the registry
 *     lock; the lock itself is a leaf — never sleep or issue IPC under it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/errno.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proc.h>
#include <ewoksys/shm_pipe.h>
#include "piped.h"

#define MAX_PIPES        256
#define MAX_PIPE_WAITERS 16

#define PIPE_END_READ  1
#define PIPE_END_WRITE 2

typedef struct {
    bool used;
    int32_t pid;
    uint32_t uuid;      /* generation guard: pids recycle fast */
    uint32_t events;    /* VFS_EVT_RD/WR interests */
} pipe_waiter_t;

typedef struct {
    bool used;
    int32_t shm_id;             /* also the pipe id shipped in fsinfo.data */
    shm_pipe_t* ring;
    ewokos_addr_t read_node;
    ewokos_addr_t write_node;   /* 0 while the write end is not attached */
    int32_t read_refs;
    int32_t write_refs;
    pipe_waiter_t waiters[MAX_PIPE_WAITERS];
} pipe_entry_t;

static pipe_entry_t _pipes[MAX_PIPES];
static pthread_mutex_t _pipes_lock;

/* wake snapshot: filled under _pipes_lock, delivered after unlocking */
#define WAKE_MAX (MAX_PIPE_WAITERS + 1)
typedef struct {
    int num;
    int32_t pid[WAKE_MAX];
} wake_snap_t;

static void wake_snap_add(wake_snap_t* snap, int32_t pid) {
    if(pid > 0 && snap->num < WAKE_MAX)
        snap->pid[snap->num++] = pid;
}

static void wake_snap_fire(wake_snap_t* snap, ewokos_addr_t token) {
    for(int i = 0; i < snap->num; i++)
        proc_wakeup_by(snap->pid[i], token);
}

/* callers must hold _pipes_lock */
static pipe_entry_t* entry_by_pipe(int32_t shm_id) {
    for(int i = 0; i < MAX_PIPES; i++) {
        if(_pipes[i].used && _pipes[i].shm_id == shm_id)
            return &_pipes[i];
    }
    return NULL;
}

/* callers must hold _pipes_lock; returns PIPE_END_READ/WRITE or 0 */
static pipe_entry_t* entry_by_node(ewokos_addr_t node, int* end) {
    for(int i = 0; i < MAX_PIPES; i++) {
        pipe_entry_t* e = &_pipes[i];
        if(!e->used)
            continue;
        if(node != 0 && e->read_node == node) {
            *end = PIPE_END_READ;
            return e;
        }
        if(node != 0 && e->write_node == node) {
            *end = PIPE_END_WRITE;
            return e;
        }
    }
    *end = 0;
    return NULL;
}

/*
 * Drop a waiter whose owner is gone or recycled. uuid==0 (dead pid) or a
 * mismatched uuid proves the registration outlived its process; leaving it
 * in would both leak has_poll_waiters and let PIPE_EDGE fire stale wakes.
 * callers must hold _pipes_lock.
 */
static void waiter_reap_stale(pipe_entry_t* e) {
    for(int i = 0; i < MAX_PIPE_WAITERS; i++) {
        pipe_waiter_t* w = &e->waiters[i];
        if(!w->used)
            continue;
        if(proc_get_uuid(w->pid) != w->uuid) {
            w->used = false;
            if(__atomic_load_n(&e->ring->has_poll_waiters, __ATOMIC_RELAXED) > 0)
                __atomic_sub_fetch(&e->ring->has_poll_waiters, 1, __ATOMIC_RELAXED);
        }
    }
}

/*
 * Snapshot the pids of waiters interested in 'events'. Stale entries are
 * reaped on the way. callers must hold _pipes_lock.
 */
static void waiters_snapshot(pipe_entry_t* e, uint32_t events, wake_snap_t* snap) {
    waiter_reap_stale(e);
    for(int i = 0; i < MAX_PIPE_WAITERS; i++) {
        pipe_waiter_t* w = &e->waiters[i];
        if(w->used && (w->events & events) != 0)
            wake_snap_add(snap, w->pid);
    }
}

/*
 * The pipe ring must NOT be an IPC_PRIVATE segment: the kernel grants
 * private shm only to the owner and its descendants (check_access
 * "family only" rule), but a pipe ring is attached by whoever holds the
 * fd — pipeline children are piped's clients, not its children — so a
 * private ring is unattachable for every non-root user (shmat fails,
 * libc read/write then dies with EIO). Use a fresh keyed 0666 segment
 * per pipe instead: the mode bits are only consulted for keyed
 * segments, so other=r+w lets any uid attach.
 *
 * Key = fingerprint | pid | counter. piped is a permanent single
 * instance, so the monotonic counter alone keeps keys unique for the
 * daemon's lifetime; the fingerprint keeps piped's key space apart from
 * other keyed-shm users (graph/g2dd), the pid distinguishes a restarted
 * instance, and IPC_EXCL turns a post-wraparound collision into a retry
 * instead of silently returning an existing segment WITHOUT resizing it.
 */
#define PIPE_SHM_KEY_FP   0x50000000u /* 'P' << 24: piped key-space fingerprint */
#define PIPE_SHM_KEY_RETRIES 16       

static int32_t pipe_shm_alloc(void) {
    static uint32_t _key_seq = 0;

    for(int i = 0; i < PIPE_SHM_KEY_RETRIES; i++) {
        key_t key = (key_t)(PIPE_SHM_KEY_FP |
                (((uint32_t)getpid() & 0xffu) << 16) | (_key_seq & 0xffffu));
        _key_seq++;
        int32_t id = shmget(key, SHM_PIPE_PAGE_SIZE, IPC_CREAT | IPC_EXCL | 0666);
        if(id > 0)
            return id;
    }
    return -1;
}

static int pipe_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int oflag, void* p) {
    (void)dev; (void)fd; (void)from_pid; (void)p;

    if((oflag & O_ACCMODE) == O_WRONLY) {
        /*
         * Unattached write end: pipe() binds it to the pipe created by the
         * read-end open via PIPE_ATTACH. No registry state yet — a close
         * before attach simply finds nothing. Advertise FS_TYPE_PIPE already
         * so libc routes this fd through the shm pipe path once attached.
         */
        info->type = FS_TYPE_PIPE;
        info->data = 0;
        return 0;
    }
    if((oflag & O_ACCMODE) != O_RDONLY) {
        errno = EINVAL; /* pipes are strictly one-directional */
        return -1;
    }

    /* shm allocation is a kernel round trip: do it outside _pipes_lock */
    int32_t shm_id = pipe_shm_alloc();
    if(shm_id <= 0) {
        errno = ENOMEM;
        return -1;
    }
    shm_pipe_t* ring = (shm_pipe_t*)shmat(shm_id, NULL, 0);
    if(ring == (void*)-1) {
        shmctl(shm_id, IPC_RMID, NULL);
        errno = ENOMEM;
        return -1;
    }
    /* The read end's node id doubles as the pipe-wide block token: both
     * ends and every poll waiter sleep/wake on ring->node_id. */
    shm_pipe_init(ring, (int32_t)info->node, shm_id);

    pthread_mutex_lock(&_pipes_lock);
    pipe_entry_t* e = NULL;
    for(int i = 0; i < MAX_PIPES; i++) {
        if(!_pipes[i].used) {
            e = &_pipes[i];
            break;
        }
    }
    if(e == NULL) {
        pthread_mutex_unlock(&_pipes_lock);
        shmdt(ring);
        shmctl(shm_id, IPC_RMID, NULL);
        errno = ENFILE;
        return -1;
    }
    memset(e, 0, sizeof(pipe_entry_t));
    e->used = true;
    e->shm_id = shm_id;
    e->ring = ring;
    e->read_node = info->node;
    e->read_refs = 1;
    pthread_mutex_unlock(&_pipes_lock);

    /* Advertise the pipe identity on this fd: FS_TYPE_PIPE makes libc use
     * the shm fast path, fsinfo.data carries the shm id for lazy shmat()
     * after fork and as the PIPE_ATTACH key for the write end. */
    info->type = FS_TYPE_PIPE;
    info->data = (uint32_t)shm_id;
    return 0;
}

static int pipe_dup(vdevice_t* dev, int from_fd, int from_pid, int dup_fd, int dup_pid,
        ewokos_addr_t node, fsinfo_t* fsinfo, void* p) {
    (void)dev; (void)from_fd; (void)from_pid; (void)dup_fd; (void)dup_pid;
    (void)fsinfo; (void)p;

    /* vfsd sends exactly one FS_CMD_DUP per ref-owning inherited fd (fork
     * clone) and per same-process dup/dup2 of a pipe fd (the dup'd end owns
     * its own ref: it routinely outlives its source descriptor). */
    int end = 0;
    pthread_mutex_lock(&_pipes_lock);
    pipe_entry_t* e = entry_by_node(node, &end);
    if(e != NULL) {
        if(end == PIPE_END_READ) {
            if(++e->read_refs > 0) {
                /* Revive: dup2 overwriting a pipe fd queues the victim's
                 * driver close as an async task; that -1 can land before
                 * this +1. Clearing the flag here is safe — while any end
                 * descriptor is alive no peer may observe a closed edge. */
                __atomic_exchange_n(&e->ring->reader_closed, 0,
                        __ATOMIC_ACQ_REL);
            }
        }
        else {
            if(++e->write_refs > 0)
                __atomic_exchange_n(&e->ring->writer_closed, 0,
                        __ATOMIC_ACQ_REL);
        }
    }
    pthread_mutex_unlock(&_pipes_lock);
    return 0;
}

static int pipe_close(vdevice_t* dev, int fd, int from_pid, ewokos_addr_t node,
        fsinfo_t* fsinfo, void* p) {
    (void)dev; (void)fd; (void)fsinfo; (void)p;

    int end = 0;
    shm_pipe_t* ring = NULL;
    int32_t shm_id = 0;
    int32_t stamp_pid = 0;
    wake_snap_t snap = {0};
    bool destroy = false;

    pthread_mutex_lock(&_pipes_lock);
    pipe_entry_t* e = entry_by_node(node, &end);
    if(e == NULL) {
        /* unattached write end (or stale duplicate close): nothing to do */
        pthread_mutex_unlock(&_pipes_lock);
        return 0;
    }
    ring = e->ring;
    shm_id = e->shm_id;

    if(end == PIPE_END_READ) {
        e->read_refs--;
        /* Retire the closer's block stamp: a task killed while blocked never
         * clears it itself, and a surviving stamp would later make a peer
         * fire proc_wakeup_by() at a recycled pid. CAS so a concurrent
         * legitimate consume by the counterpart wins cleanly. */
        int32_t expect = from_pid;
        __atomic_compare_exchange_n(&ring->reader_pid, &expect, 0,
                false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
        if(e->read_refs <= 0) {
            __atomic_store_n(&ring->reader_closed, 1, __ATOMIC_RELEASE);
            /* one-shot wake for a shm-path writer blocked on EPIPE */
            stamp_pid = __atomic_exchange_n(&ring->writer_pid, 0, __ATOMIC_ACQUIRE);
            /*
             * Close is pipe-wide, never interest-filtered: wake every waiter.
             * A poll(POLLIN)-only waiter must still wake to observe POLLHUP,
             * and an events==0 registration can never match an interest mask.
             */
            waiters_snapshot(e, VFS_EVT_RW | VFS_EVT_CLOSE | VFS_EVT_ERR | VFS_EVT_NVAL, &snap);
        }
    }
    else {
        e->write_refs--;
        int32_t expect = from_pid;
        __atomic_compare_exchange_n(&ring->writer_pid, &expect, 0,
                false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
        if(e->write_refs <= 0) {
            __atomic_store_n(&ring->writer_closed, 1, __ATOMIC_RELEASE);
            /* one-shot wake for a shm-path reader blocked on EOF */
            stamp_pid = __atomic_exchange_n(&ring->reader_pid, 0, __ATOMIC_ACQUIRE);
            /* see the read-end close above: close wakes every waiter */
            waiters_snapshot(e, VFS_EVT_RW | VFS_EVT_CLOSE | VFS_EVT_ERR | VFS_EVT_NVAL, &snap);
        }
    }

    if(e->read_refs <= 0 && e->write_refs <= 0) {
        e->used = false;
        destroy = true;
    }
    pthread_mutex_unlock(&_pipes_lock);

    ewokos_addr_t token = (ewokos_addr_t)ring->node_id;
    if(stamp_pid > 0)
        proc_wakeup_by(stamp_pid, token);
    wake_snap_fire(&snap, token);

    if(destroy) {
        shmdt(ring);
        shmctl(shm_id, IPC_RMID, NULL);
    }
    return 0;
}

static int pipe_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int cmd,
        proto_t* in, proto_t* out, void* p) {
    (void)dev; (void)fd; (void)out; (void)p;

    switch(cmd) {
    case PIPE_ATTACH: {
        int32_t shm_id = proto_read_int(in);
        if(shm_id <= 0 || info->data != 0) {
            errno = EINVAL;
            return -1; /* bad pipe id or this end is already attached */
        }
        pthread_mutex_lock(&_pipes_lock);
        pipe_entry_t* e = entry_by_pipe(shm_id);
        if(e == NULL || e->write_node != 0) {
            pthread_mutex_unlock(&_pipes_lock);
            errno = ENOENT;
            return -1; /* pipe already gone, or write end taken */
        }
        e->write_node = info->node;
        e->write_refs = 1;
        pthread_mutex_unlock(&_pipes_lock);
        info->data = (uint32_t)shm_id;
        return 0;
    }
    case PIPE_POLL_WAIT: {
        uint32_t events = (uint32_t)proto_read_int(in);
        uint32_t uuid = proc_get_uuid(from_pid);
        if(uuid == 0)
            return -1;

        int end = 0;
        pthread_mutex_lock(&_pipes_lock);
        pipe_entry_t* e = entry_by_node(info->node, &end);
        if(e == NULL) {
            pthread_mutex_unlock(&_pipes_lock);
            return -1;
        }
        /* one waiter slot per pid: merge repeated registrations */
        pipe_waiter_t* slot = NULL;
        for(int i = 0; i < MAX_PIPE_WAITERS; i++) {
            if(e->waiters[i].used && e->waiters[i].pid == from_pid) {
                e->waiters[i].uuid = uuid;
                e->waiters[i].events |= events;
                slot = &e->waiters[i];
                break;
            }
        }
        if(slot == NULL) {
            for(int i = 0; i < MAX_PIPE_WAITERS; i++) {
                if(!e->waiters[i].used) {
                    e->waiters[i].used = true;
                    e->waiters[i].pid = from_pid;
                    e->waiters[i].uuid = uuid;
                    e->waiters[i].events = events;
                    __atomic_add_fetch(&e->ring->has_poll_waiters, 1, __ATOMIC_RELAXED);
                    slot = &e->waiters[i];
                    break;
                }
            }
        }
        pthread_mutex_unlock(&_pipes_lock);
        if(slot == NULL) {
            errno = ENOSPC;
            return -1;
        }
        return 0;
    }
    case PIPE_POLL_UNWAIT: {
        int end = 0;
        pthread_mutex_lock(&_pipes_lock);
        pipe_entry_t* e = entry_by_node(info->node, &end);
        if(e != NULL) {
            for(int i = 0; i < MAX_PIPE_WAITERS; i++) {
                pipe_waiter_t* w = &e->waiters[i];
                if(w->used && w->pid == from_pid) {
                    w->used = false;
                    if(__atomic_load_n(&e->ring->has_poll_waiters, __ATOMIC_RELAXED) > 0)
                        __atomic_sub_fetch(&e->ring->has_poll_waiters, 1, __ATOMIC_RELAXED);
                    break;
                }
            }
        }
        pthread_mutex_unlock(&_pipes_lock);
        return 0;
    }
    case PIPE_EDGE: {
        uint32_t events = (uint32_t)proto_read_int(in);
        int end = 0;
        wake_snap_t snap = {0};
        ewokos_addr_t token = 0;

        pthread_mutex_lock(&_pipes_lock);
        pipe_entry_t* e = entry_by_node(info->node, &end);
        if(e != NULL) {
            waiters_snapshot(e, events & VFS_EVT_RW, &snap);
            token = (ewokos_addr_t)e->ring->node_id;
        }
        pthread_mutex_unlock(&_pipes_lock);

        wake_snap_fire(&snap, token);
        return 0;
    }
    }
    errno = EINVAL;
    return -1;
}

static uint32_t pipe_check_poll_events(vdevice_t* dev, int fd, int from_pid,
        fsinfo_t* info, void* p) {
    (void)dev; (void)fd; (void)from_pid; (void)p;

    /* Fallback live-state source. The normal poll path reads the ring
     * directly in libc; this only runs if vfsd forwards FS_CMD_POLL. */
    int end = 0;
    uint32_t events = 0;
    pthread_mutex_lock(&_pipes_lock);
    pipe_entry_t* e = entry_by_node(info->node, &end);
    if(e != NULL) {
        shm_pipe_t* ring = e->ring;
        if(shm_pipe_readable(ring) > 0)
            events |= VFS_EVT_RD;
        if(shm_pipe_writable(ring) > 0)
            events |= VFS_EVT_WR;
        if(__atomic_load_n(&ring->writer_closed, __ATOMIC_ACQUIRE) ||
                __atomic_load_n(&ring->reader_closed, __ATOMIC_ACQUIRE))
            events |= VFS_EVT_CLOSE;
    }
    pthread_mutex_unlock(&_pipes_lock);
    return events;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1] : PIPE_DEV_PATH;

    pthread_mutex_init(&_pipes_lock, NULL);

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "pipe");
    dev.open = pipe_open;
    dev.close = pipe_close;
    dev.dup = pipe_dup;
    dev.fcntl = pipe_fcntl;
    dev.check_poll_events = pipe_check_poll_events;

    device_run(&dev, mnt_point, FS_TYPE_ANNOUNIMOUS | FS_TYPE_CHAR, 0666, false);
    return 0;
}
