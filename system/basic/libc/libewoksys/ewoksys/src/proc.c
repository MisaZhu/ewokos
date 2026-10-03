#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/ipc_serv.h>
#include <ewoksys/vfs.h>
#include <ewoksys/syscall.h>
#include <ewoksys/sys.h>
#include <ewoksys/kernel_tic.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/core.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/shm.h>

#ifdef __cplusplus
extern "C" {
#endif

bool _proc_global_need_lock = false;
static pthread_mutex_t _proc_global_lock = 0;
static int _reent_dep = 0;

int _vfsd_pid = -1;
int _cored_pid = -1;
static int _lock_thread = -1;

void proc_init(void) {
    _vfsd_pid = -1;
    _cored_pid = -1;
    _lock_thread = -1;
    _proc_global_need_lock = false;
    _reent_dep = 0;
    _proc_global_lock = 0;
}

void proc_exit(void) {
    if(_proc_global_lock != 0)
        pthread_mutex_destroy(&_proc_global_lock);
}

void proc_priority(uint32_t pid, uint32_t priority) {
    syscall2(SYS_PROC_PRIORITY, pid, priority);
}

void proc_malloc_lock_prepare(void) {
    /*
     * Allocate the malloc-guard mutex eagerly while the process is still
     * single-threaded (called from thread_create() right before the first
     * child thread is spawned). Lazy-initializing it from proc_global_lock()
     * races when the first two threads malloc at the same time: both observe
     * _proc_global_lock == 0, each semaphore_alloc()s a different id, and the
     * losing writer's semaphore is leaked while the two threads briefly guard
     * the heap with different locks -> free-list corruption at startup.
     */
    if(_proc_global_lock == 0)
        pthread_mutex_init(&_proc_global_lock, NULL);
}

static inline void proc_global_lock(void) {
    int tid = pthread_self();
    if(tid == _lock_thread) {
        _reent_dep++;
        return;
    }

    if(_proc_global_lock == 0)
        pthread_mutex_init(&_proc_global_lock, NULL);

    pthread_mutex_lock(&_proc_global_lock);
    _lock_thread = tid;
}

static inline void proc_global_unlock(void) {
    int tid = pthread_self();
    if(tid == _lock_thread) {
        if(_reent_dep > 0)  {
            _reent_dep--;
            return;
        }
    }

    /*
     * Clear ownership BEFORE releasing the mutex.
     *
     * If _lock_thread were cleared AFTER pthread_mutex_unlock(), a thread
     * blocked in proc_global_lock() could acquire the mutex in that gap and
     * set _lock_thread to itself, and our trailing "_lock_thread = -1" would
     * then wipe the new owner. That new owner's next nested malloc/free would
     * read _lock_thread == -1, fail the "already mine" check, and call
     * semaphore_enter() again on a semaphore it already holds -> self-deadlock
     * (the netd worker hangs, IPC FS_CMD_POLL times out) while the malloc heap
     * is left half-updated. Under concurrent load (multiple worker threads all
     * growing proto_t buffers) this corrupts the free list and faults with a
     * jump to an unmapped address. Clearing under the lock closes the window.
     */
    _lock_thread = -1;
    pthread_mutex_unlock(&_proc_global_lock);
}

void __malloc_lock (struct _reent *reent) {
    if(!_proc_global_need_lock)
        return;
    proc_global_lock();
}

void __malloc_unlock (struct _reent *reent) {
    if(!_proc_global_need_lock)
        return;
    proc_global_unlock();
}

inline int get_vfsd_pid(void) {
    if(_vfsd_pid < 0)
        _vfsd_pid = ipc_serv_get(IPC_SERV_VFS);
    return _vfsd_pid;
}

inline int get_cored_pid(void) {
    if(_cored_pid < 0)
        _cored_pid = syscall0(SYS_CORE_PID);
    return _cored_pid;
}

inline int proc_getpid(int pid) {
    if(pid < 0) {
        if(_current_pid < 0)
            _current_pid = syscall1(SYS_GET_PID, (ewokos_addr_t)pid);
        return _current_pid;
    }	

    if(_current_pid == pid)
        return pid;
    
    if(pid >= MAX_PROC_NUM)
        return -1;

    /*
     * The vsyscall table is mutated lock-free by the kernel, and the
     * multi_task IPC pool churns thread slots constantly (spawn on demand,
     * self-quit when idle), so a torn read can produce a father_pid cycle
     * or an out-of-range link. Bound the walk and range-check every hop:
     * degrading to "unknown owner" is harmless, while an infinite spin
     * here wedges the caller - vfsd runs this under _vfs_lock(write) in
     * every fd op (open/close/dup/dup2/clone), so a spin deadlocks the
     * whole system.
     */
    uint32_t steps = 0;
    while(steps++ < MAX_PROC_NUM) {
        if(_vsyscall_info->proc_info[pid].uuid == 0)
            return -1;

        if(_vsyscall_info->proc_info[pid].type == TASK_TYPE_PROC)
            return pid;
        pid = _vsyscall_info->proc_info[pid].father_pid;
        if(pid < 0 || pid >= MAX_PROC_NUM)
            return -1;
    }
    return -1;
}

int proc_getpid_or_raw(int pid) {
    int owner = proc_getpid(pid);
    if(owner < 0)
        owner = pid;
    return owner;
}

inline int proc_fork(void) {
    int ret = syscall0(SYS_FORK);
    if(ret == 0) {
        _current_pid = -1;
        proc_getpid(-1);
        vfs_on_fork();
    }
    return ret;
}

inline int proc_info(int pid, procinfo_t* info) {
    return syscall2(SYS_GET_PROC, (ewokos_addr_t)pid, (ewokos_addr_t)info);
}

inline void proc_detach(void) {
    syscall0(SYS_DETACH);
}

inline void proc_block(void) {
    syscall1(SYS_BLOCK, 0);
}

/**
 * @brief block the current proc waiting on a specific VFS node token.
 *
 * A non-zero token scopes the block so only a wakeup for the same node (or a
 * generic tokenless wakeup) can release it. This prevents an event on an
 * unrelated node from spuriously waking this proc.
 */
inline void proc_block_by(ewokos_addr_t token) {
    syscall1(SYS_BLOCK, (ewokos_addr_t)token);
}

/**
 * @brief block the current proc on a VFS node token with a deadline.
 *
 * Poll-emulation block: the kernel releases the proc when a matching wake
 * arrives OR when timeout_usec elapses (timeout_usec == 0 blocks forever).
 * Unlike proc_block_by(), a latched generic (token-0) wake from an ipc_call
 * round-trip is discarded instead of satisfying the block, so registration
 * IPCs cannot turn the wait into a hot probe loop.
 */
inline void proc_block_timeout(ewokos_addr_t token, uint32_t timeout_usec) {
    syscall2(SYS_BLOCK_TIMEOUT, (ewokos_addr_t)token, (ewokos_addr_t)timeout_usec);
}

/**
 * @brief wakeup process by pid
 * 
 * @param pid wakeup blocked process pid
 */
inline void proc_wakeup(int32_t pid) {
    syscall2(SYS_WAKEUP, (ewokos_addr_t)pid, 0);
}

/**
 * @brief wakeup process by pid, scoped to a VFS node token.
 *
 * Only releases the target proc if it is blocked on this token or blocked
 * generically; a proc blocked on a different node is left untouched.
 */
inline void proc_wakeup_by(int32_t pid, ewokos_addr_t token) {
    syscall2(SYS_WAKEUP, (ewokos_addr_t)pid, (ewokos_addr_t)token);
}


inline void proc_exec_elf(const char* cmd_line, int32_t shm_id, int32_t size) {
    if(syscall3(SYS_EXEC_ELF, (ewokos_addr_t)cmd_line, (ewokos_addr_t)shm_id, (ewokos_addr_t)size) != 0)
        exit(-1);
}

static void saveenv() {
    char buf[256] = {0};
    char ** env;
    extern char ** environ;
    env = environ;
    for (env; *env; ++env) {
        memset(buf, 0, sizeof(buf));
        char* key = buf;
        char* value = buf;
        for(int i= 0;i < sizeof(buf)-1; i++){
            char c = (*env)[i];
            if(c == '\0'){
                break;
            }
            else if(c == '='){
                buf[i] = '\0';
                value = &buf[i+1];
            }else{
                buf[i] = c;
            }
        }
        if(key[0] != 0){
            core_set_env(key, value);
        }
  }
}

static void close_on_exec_fds(void) {
    for(int fd = 0; fd < MAX_OPEN_FILE_PER_PROC; fd++) {
        int flags = vfs_get_fd_flags(fd);
        if(flags >= 0 && (flags & FD_CLOEXEC) != 0) {
            vfs_close(fd);
        }
    }
}

int proc_exec(const char *name) {
    char fpath[64];
    int sz = 0;
    const char *p = name;
    saveenv();
    memset(fpath, 0, sizeof(fpath));
    for(int i = 0; i < sizeof(fpath); i++){
        if(name[i] == '\0' || name[i] == ' ' || name[i] == '\t' || name[i] == '\n')
            break;
        fpath[i] = name[i];
    }

    /* Get file size first */
    fsinfo_t info;
    if(vfs_get_by_name(fpath, &info) != 0 || info.stat.size <= 0)
        return -1;
    sz = (int)info.stat.size;

    /* Allocate shm and read file directly into it */
    int shm_id = shmget(0, sz, 0666);
    if(shm_id <= 0)
        return -1;
    uint8_t* shm_buf = (uint8_t*)shmat(shm_id, NULL, 0);
    if(shm_buf == NULL) {
        shmctl(shm_id, IPC_RMID, NULL);
        return -1;
    }

    int rd = vfs_readfile_to_buf(fpath, shm_buf, sz);
    if(rd < 0) {
        shmdt(shm_buf);
        shmctl(shm_id, IPC_RMID, NULL);
        return -1;
    }

    close_on_exec_fds();
    proc_exec_elf(name, shm_id, sz);
    /* If exec fails, clean up shm */
    shmdt(shm_buf);
    shmctl(shm_id, IPC_RMID, NULL);
    return 0;
}

inline uint32_t proc_check_uuid(int32_t pid, uint32_t uuid) {
    if(pid < 0 || pid >= MAX_PROC_NUM)
        return 0;

    if(_vsyscall_info->proc_info[pid].uuid == uuid)
        return uuid;
    return 0;
}

inline uint32_t proc_get_uuid(int32_t pid) {
    if(pid < 0 || pid >= MAX_PROC_NUM)
        return 0;
    return _vsyscall_info->proc_info[pid].uuid;
}

inline void* proc_malloc_expand(int64_t size) {
    //klog("proc_malloc_expand %d, pid: %d\n", size, getpid());
    return (void*)syscall1(SYS_MALLOC_EXPAND, (ewokos_addr_t)size);
}

inline void proc_malloc_free(void) {
    //klog("proc_free, pid: %d\n", getpid());
    syscall0(SYS_FREE);
}

inline uint32_t proc_malloc_size(void) {
    return syscall0(SYS_MALLOC_SIZE);
}

/*
 * Largest tick period proc_nsleep_precise() will trust as a spin budget, and
 * the budget it clamps to when the measured tick is larger or not yet known.
 * Reserving costs busy CPU per call, so on platforms with a coarse tick
 * (virt.riscv 4096us, lego.ev3 3906us, x86 10000us) the budget is capped here
 * instead of tracking the tick: a long sleep then overshoots its coarse part by
 * up to a tick, which the absolute spin target absorbs, and the spin itself
 * stays bounded. Platforms with no user-readable counter at all (x86, lego.ev3)
 * never reach this and keep tick resolution.
 */
#define PROC_SLEEP_SPIN_MAX_US   1200

/*
 * Hard iteration cap on the spin loop. If the fine clock ever stops advancing -
 * a counter the kernel published that this libc cannot actually read, or a stuck
 * timer - this converts an infinite busy loop into a normal sleep syscall.
 * 2e6 iterations is ~25x the worst case a PROC_SLEEP_SPIN_MAX_US window needs,
 * so it never trips on healthy hardware.
 */
#define PROC_SLEEP_SPIN_ITERS    2000000

/* keeps the coarse part of a long sleep inside a uint32 usec count */
#define PROC_SLEEP_CHUNK_US      60000000

/* coarse sleep via the kernel; chunked so a uint64 nsec request cannot wrap */
static void proc_usleep_syscall(uint64_t usecs) {
    while(usecs > 0) {
        uint32_t chunk = (usecs > PROC_SLEEP_CHUNK_US) ? PROC_SLEEP_CHUNK_US : (uint32_t)usecs;
        syscall1(SYS_USLEEP, (ewokos_addr_t)chunk);
        usecs -= chunk;
    }
}

/*
 * Precise sleep: coarse syscall for the bulk, exact spin for the tail.
 *
 * Spinning the whole duration would be accurate but holds a core, which is
 * unacceptable beyond a few hundred microseconds. Instead sleep for
 * (nsecs - one tick) and spin out the rest. SYS_USLEEP can only overshoot its
 * request, never undershoot, so on return at least the reserved tick is still
 * owed: the spin is guaranteed shorter than one tick and the wake still lands
 * on the nanosecond. Net cost for any sleep longer than a tick is exactly the
 * one syscall it already took, plus a sub-tick busy tail - no extra context
 * switches.
 *
 * The spin runs against the raw hardware counter (kernel_tic_spin_until), not
 * against kernel_tic_nsec(). The interpolated clock re-derives its offset from
 * real time every time the kernel republishes its base, and a republish that
 * shrinks the offset moves the clock forward - which would let the spin reach
 * its deadline before the time had actually elapsed. Measuring the counter
 * delta directly has no such discontinuity.
 *
 * Returns -1 when this platform cannot do better than tick resolution (no
 * user-readable counter, or a tick too coarse to reserve), leaving the caller
 * to fall back.
 */
int proc_nsleep_precise(uint64_t nsecs) {
    uint32_t tick_usec;
    uint64_t spin_ns;
    uint64_t start_cnt;

    if(nsecs == 0)
        return 0;

    if(!kernel_tic_fine_available())
        return -1;

    /*
     * Clamp the spin budget rather than giving up when the measured tick is out
     * of range. tick_usec is republished from the *measured* gap on every tick,
     * so a single late tick - routine under a hypervisor, where the host can
     * deschedule the guest for milliseconds - would otherwise inflate spin_ns
     * to the measured gap and turn every sleep up to that length into a pure
     * busy-spin holding a core for milliseconds.
     *
     * Clamping costs nothing: the budget only bounds how much of a long sleep is
     * spun, and overshooting a coarse sleep is absorbed by the absolute spin
     * target below. Being late is recoverable; being early is not.
     */
    tick_usec = kernel_tic_tick_usec();
    if(tick_usec == 0 || tick_usec > PROC_SLEEP_SPIN_MAX_US)
        tick_usec = PROC_SLEEP_SPIN_MAX_US;

    /*
     * Snapshot before anything can block: the spin target is absolute, so
     * whatever the coarse sleep overshoots by comes off the spin rather than
     * being added to the total.
     */
    if(kernel_tic_fine_cnt(&start_cnt) != 0)
        return -1;

    spin_ns = (uint64_t)tick_usec * 1000ULL;
    if(nsecs > spin_ns)
        proc_usleep_syscall((nsecs - spin_ns) / 1000ULL);

    if(kernel_tic_spin_until(start_cnt, nsecs, PROC_SLEEP_SPIN_ITERS) == 0)
        return 0;

    /*
     * The spin cap tripped, so the counter is not advancing as expected. Finish
     * with a syscall rather than returning early - being late is recoverable,
     * being early is not.
     */
    proc_usleep_syscall(nsecs / 1000ULL + 1);
    return 0;
}

#ifdef __cplusplus
}
#endif
