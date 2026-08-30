/*
 * driver_async.c - async driver worker thread (close/dup/kids jobs).
 *
 * Outbound IPC to mount drivers (FS_CMD_CLOSE/DUP/KIDS) must never happen
 * while _vfs_lock is held (see the lock rules in vfsd.h), so all of it is
 * deferred to this single worker thread plus a guarded results queue. Also
 * hosts the directory kids lazy-load machinery that builds on it.
 */
#include "vfsd.h"

driver_async_worker_t _driver_async_worker = {0};
queue_t _driver_kids_results;
pthread_mutex_t _driver_kids_results_lock = 0;

static void driver_async_process_close_job(driver_close_task_t* job);
static void driver_async_process_dup_job(driver_dup_job_t* job);
static void driver_async_process_kids_job(driver_kids_job_t* job);

static void* driver_async_worker_entry(void* arg) {
    driver_async_worker_t* worker = (driver_async_worker_t*)arg;
    worker->thread = pthread_self();

    while(true) {
        void* job = NULL;
        driver_async_job_type_t type = DRIVER_ASYNC_JOB_NONE;

        pthread_mutex_lock(&worker->lock);
        job = queue_pop(&worker->jobs);
        pthread_mutex_unlock(&worker->lock);
        if(job == NULL) {
            proc_block_by(VFSD_WAKE_TOKEN_DRIVER_ASYNC);
            continue;
        }

        type = *((driver_async_job_type_t*)job);
        if(type == DRIVER_ASYNC_JOB_CLOSE)
            driver_async_process_close_job((driver_close_task_t*)job);
        else if(type == DRIVER_ASYNC_JOB_DUP)
            driver_async_process_dup_job((driver_dup_job_t*)job);
        else if(type == DRIVER_ASYNC_JOB_KIDS)
            driver_async_process_kids_job((driver_kids_job_t*)job);
        else
            free(job);
    }

    return NULL;
}

void start_driver_async_worker(void) {
    pthread_t tid;

    pthread_mutex_lock(&_driver_async_worker.lock);
    if(_driver_async_worker.started == 0) {
        if(pthread_create(&tid, NULL, driver_async_worker_entry, &_driver_async_worker) == 0) {
            pthread_detach(tid);
            _driver_async_worker.thread = tid;
            _driver_async_worker.started = 1;
        }
    }
    pthread_mutex_unlock(&_driver_async_worker.lock);
}

/*
 * Outbound FS_CMD_CLOSE. Normally only ever called from the async driver
 * worker thread; the dup2 pipe-victim path calls it directly from a handler
 * UNLOCKED phase to keep the -1 ahead of the paired FS_CMD_DUP. Never call
 * while _vfs_lock is held (see lock rules in vfsd.h).
 */
void vfs_driver_close(int32_t pid, int32_t owner_pid, int32_t fd, file_t* file) {
    if(file == NULL)
        return;
    uint32_t type = FS_BASE_TYPE(file->fsinfo.type);
    /*
     * Regular filesystem objects in rootfs do not keep per-fd runtime state in
     * the backing driver. Zombie cleanup already detached the VFS-side slot, so
     * round-tripping a no-op FS_CMD_CLOSE for every inherited script/config file
     * only lengthens the window where boot-time metadata traffic contends with
     * reaping detached helpers.
     */
    if(type == FS_TYPE_FILE || type == FS_TYPE_DIR || type == FS_TYPE_LINK)
        return;

    int32_t mount_pid = file->fsinfo.mount_pid;
    if(mount_pid <= 0 || file->fsinfo.node == 0)
        return;

    proto_t in;
    PF->format(&in, "i,i,m,i,i",
        (ewokos_addr_t)fd, file->fsinfo.node, &file->fsinfo, sizeof(fsinfo_t),
        (ewokos_addr_t)pid, (ewokos_addr_t)owner_pid);
    ipc_call(mount_pid, FS_CMD_CLOSE, &in, NULL);
    PF->clear(&in);
}

static void driver_async_process_close_job(driver_close_task_t* job) {
    if(job == NULL)
        return;
    vfs_driver_close(job->pid, job->owner_pid, job->fd, &job->file);
    free(job);
}

/* may be called while holding _vfs_lock (write): only touches the worker queue */
void enqueue_driver_close_task(driver_close_task_t* task) {
    pthread_t tid;

    if(task == NULL)
        return;

    task->job_type = DRIVER_ASYNC_JOB_CLOSE;
    pthread_mutex_lock(&_driver_async_worker.lock);
    queue_push(&_driver_async_worker.jobs, task);
    tid = _driver_async_worker.thread;
    pthread_mutex_unlock(&_driver_async_worker.lock);
    if(tid != 0)
        proc_wakeup_by((int32_t)tid, VFSD_WAKE_TOKEN_DRIVER_ASYNC);
}

/*
 * Outbound FS_CMD_DUP. Called either from the async driver worker thread or
 * from a handler's UNLOCKED phase (fallback dups): never while _vfs_lock is
 * held.
 */
int vfs_driver_dup_now(int32_t mount_pid, int32_t from_pid, int32_t from_fd,
        int32_t dup_pid, int32_t dup_fd, const file_t* file) {
    if(file == NULL || file->node == NULL)
        return 0;
    uint32_t type = FS_BASE_TYPE(file->fsinfo.type);
    /*
     * Regular filesystem objects do not carry per-fd runtime state in their
     * mount driver. Reads and writes always pass the current offset down from
     * libc, and the generic vdevice layer can lazily rebuild its cache from the
     * VFS node info on first access. Avoid synchronously round-tripping every
     * fork/dup of rootfs script files through the backing fs server so a stuck
     * FS_CMD_DUP cannot strand the parent and child in fork.
     */
    if(type == FS_TYPE_FILE || type == FS_TYPE_DIR || type == FS_TYPE_LINK)
        return 0;

    /*
     * Whether a same-process dup still needs this round trip is decided by
     * the caller (vfs_driver_dup): most devices let the device-side cache
     * lazily clone from the surviving source fd on first access, but piped
     * counts descriptors and must see every dup. Cross-process fork/clone
     * always ships: the parent is free to close the source fd before the
     * child performs its first device I/O, and anonymous device nodes (for
     * example accepted netd sockets) keep their live per-fd runtime object
     * behind fsinfo.data — a lazy first-access clone would let the parent's
     * early close reap that runtime object before the child-side cache
     * exists.
     */

    proto_t in;
    PF->format(&in, "i,i,i,m,i,i",
        (ewokos_addr_t)from_fd, (ewokos_addr_t)dup_fd, file->fsinfo.node,
        &file->fsinfo, sizeof(fsinfo_t), (ewokos_addr_t)from_pid,
        (ewokos_addr_t)dup_pid);
    if(mount_pid > 0) {
        /*
         * Do not wait for the driver reply here. Cross-process dup is on the
         * fork/clone hot path; if netd/WLAN or any other mount server stalls,
         * vfsd must stay responsive and let the dup worker absorb that delay.
         */
        int rc = ipc_call(mount_pid, FS_CMD_DUP, &in, NULL);
        PF->clear(&in);
        return rc;
    }
    PF->clear(&in);
    return 0;
}

/* runs on the async driver worker thread, takes _vfs_lock itself */
static bool driver_dup_job_still_valid(const driver_dup_job_t* job) {
    file_t* current;
    bool valid = false;

    if(job == NULL)
        return false;
    if(job->dup_pid < 0 || job->dup_pid >= (int32_t)_max_proc_table_num)
        return false;
    if(job->dup_fd < 0 || job->dup_fd >= MAX_OPEN_FILE_PER_PROC)
        return false;

    pthread_rwlock_rdlock(&_vfs_lock);
    do {
        if(_proc_fds_table[job->dup_pid].state != RUNNING)
            break;
        if(_proc_fds_table[job->dup_pid].uuid != job->dup_uuid)
            break;

        current = &_proc_fds_table[job->dup_pid].fds[job->dup_fd];
        if(current->node == NULL)
            break;
        if(current->node != job->file.node)
            break;
        if(current->fsinfo.node != job->file.fsinfo.node)
            break;
        if(current->fsinfo.mount_pid != job->file.fsinfo.mount_pid)
            break;
        if(current->fsinfo.data != job->file.fsinfo.data)
            break;
        valid = true;
    } while(0);
    pthread_rwlock_unlock(&_vfs_lock);
    return valid;
}

/* ---- clone dup completion context ---- */

clone_dup_ctx_t* clone_dup_ctx_create(void) {
    clone_dup_ctx_t* ctx = (clone_dup_ctx_t*)calloc(1, sizeof(clone_dup_ctx_t));
    if(ctx == NULL)
        return NULL;
    pthread_mutex_init(&ctx->lock, NULL);
    ctx->refs = 1;
    return ctx;
}

static void clone_dup_ctx_ref(clone_dup_ctx_t* ctx) {
    if(ctx != NULL)
        __atomic_add_fetch(&ctx->refs, 1, __ATOMIC_RELAXED);
}

void clone_dup_ctx_unref(clone_dup_ctx_t* ctx) {
    if(ctx == NULL)
        return;
    if(__atomic_sub_fetch(&ctx->refs, 1, __ATOMIC_ACQ_REL) == 0) {
        pthread_mutex_destroy(&ctx->lock);
        free(ctx);
    }
}

static void clone_dup_ctx_complete(clone_dup_ctx_t* ctx) {
    pthread_t waiter = 0;

    if(ctx == NULL)
        return;

    pthread_mutex_lock(&ctx->lock);
    ctx->pending--;
    if(ctx->pending <= 0) {
        ctx->pending = 0;
        waiter = ctx->waiter;
        ctx->waiter = 0;
    }
    pthread_mutex_unlock(&ctx->lock);

    if(waiter != 0)
        proc_wakeup_by((int32_t)waiter, VFSD_WAKE_TOKEN_CLONE_DUP);
}

/*
 * Upper bound for waiting on driver-side FS_CMD_DUP completion. This wait
 * runs in a vfsd IPC worker context (do_vfs_proc_clone) with NO locks held,
 * but an unbounded proc_block_by() here is still a single point of failure:
 * if the target driver (netd under sshd load) does not answer - e.g. its
 * dispatch context is itself stuck on a reverse call into vfsd - the worker
 * wedges forever and the kernel's IPC timeout never fires because it freezes
 * the counter while the server is BLOCKed. Sleep-poll with backoff instead
 * of a bare proc_block_by(): the sleep is timer-driven so it always returns,
 * while clone_dup_ctx_complete()'s proc_wakeup_by() still interrupts it
 * early on the fast path.
 */
#define CLONE_DUP_WAIT_BUDGET_US   2000000U
#define CLONE_DUP_WAIT_STEP_MIN_US 500U
#define CLONE_DUP_WAIT_STEP_MAX_US 5000U

/* must be called with NO _vfs_lock held (sleeps) */
void clone_dup_ctx_wait(clone_dup_ctx_t* ctx, int32_t mount_pid) {
    uint32_t waited = 0;
    uint32_t step = CLONE_DUP_WAIT_STEP_MIN_US;

    while(true) {
        pthread_mutex_lock(&ctx->lock);
        if(ctx->pending <= 0) {
            ctx->waiter = 0;
            pthread_mutex_unlock(&ctx->lock);
            return;
        }
        if(waited >= CLONE_DUP_WAIT_BUDGET_US) {
            ctx->waiter = 0;
            pthread_mutex_unlock(&ctx->lock);
            klog("vfsd: driver dup wait timeout mount=%d, giving up (async job still queued)\n",
                    mount_pid);
            return;
        }
        ctx->waiter = pthread_self();
        pthread_mutex_unlock(&ctx->lock);
        proc_usleep(step);
        waited += step;
        step *= 2;
        if(step > CLONE_DUP_WAIT_STEP_MAX_US)
            step = CLONE_DUP_WAIT_STEP_MAX_US;
    }
}

/* may be called while holding _vfs_lock (write): only touches ctx/worker queues */
bool queue_driver_dup_job(clone_dup_ctx_t* ctx, int32_t mount_pid,
        int32_t from_pid, int32_t from_fd,
        int32_t dup_pid, int32_t dup_fd, file_t* file) {
    uint32_t type;
    driver_dup_job_t* job;
    pthread_t tid;

    if(file == NULL || file->node == NULL)
        return false;
    type = FS_BASE_TYPE(file->fsinfo.type);
    if(type == FS_TYPE_FILE || type == FS_TYPE_DIR || type == FS_TYPE_LINK)
        return false;
    if(from_pid == dup_pid)
        return false;
    if(_driver_async_worker.started == 0)
        return false;

    job = (driver_dup_job_t*)calloc(1, sizeof(driver_dup_job_t));
    if(job == NULL)
        return false;

    job->job_type = DRIVER_ASYNC_JOB_DUP;
    job->mount_pid = mount_pid;
    job->from_pid = from_pid;
    job->from_fd = from_fd;
    job->dup_pid = dup_pid;
    job->dup_fd = dup_fd;
    job->dup_uuid = _proc_fds_table[dup_pid].uuid;
    job->file = *file;
    job->ctx = ctx;

    if(ctx != NULL) {
        pthread_mutex_lock(&ctx->lock);
        ctx->pending++;
        pthread_mutex_unlock(&ctx->lock);
        clone_dup_ctx_ref(ctx); /* released by whoever completes the job */
    }
    pthread_mutex_lock(&_driver_async_worker.lock);
    queue_push(&_driver_async_worker.jobs, job);
    tid = _driver_async_worker.thread;
    pthread_mutex_unlock(&_driver_async_worker.lock);
    if(tid != 0)
        proc_wakeup_by((int32_t)tid, VFSD_WAKE_TOKEN_DRIVER_ASYNC);
    return true;
}

static void driver_async_process_dup_job(driver_dup_job_t* job) {
    if(job == NULL)
        return;

    if(!driver_dup_job_still_valid(job)) {
        if(job->ctx != NULL) {
            clone_dup_ctx_complete(job->ctx);
            clone_dup_ctx_unref(job->ctx);
        }
        free(job);
        return;
    }

    if(vfs_driver_dup_now(job->mount_pid, job->from_pid, job->from_fd,
            job->dup_pid, job->dup_fd, &job->file) != 0) {
        klog("vfsd: driver dup failed mount=%d from=%d:%d dup=%d:%d node=%u\n",
                job->mount_pid, job->from_pid, job->from_fd,
                job->dup_pid, job->dup_fd, job->file.fsinfo.node);
    }

    if(job->ctx != NULL) {
        clone_dup_ctx_complete(job->ctx);
        clone_dup_ctx_unref(job->ctx);
    }
    free(job);
}

/*
 * Notify the mount driver of a same-process dup/dup2 (VFS_DUP/VFS_DUP2
 * handlers). Must be called with NO _vfs_lock held.
 *
 * Only pipes need this: piped refcounts descriptors to derive reader/writer
 * closed, and a dup'd pipe end routinely outlives its source fd (shell
 * pipelines do dup2(pipe,0/1) then close the original). Every other device
 * keeps the lazy first-access clone design — no round trip here.
 *
 * The round trip is synchronous by design and must stay that way: piped's
 * refcount only stays consistent if the +1 lands before the caller can
 * issue any further close on this pipe (libc sends FS_CMD_CLOSE directly
 * from user space, bypassing every vfsd queue). Returns non-zero if the
 * driver never got the +1; the caller then drops driver_ref on the new fd
 * so exit cleanup does not ship an unmatched CLOSE.
 */
int vfs_driver_dup(int32_t from_pid, int32_t from_fd,
        int32_t dup_pid, int32_t dup_fd, file_t* file) {
    if(file == NULL || file->node == NULL)
        return 0;
    if(!FS_IS_TYPE(file->fsinfo.type, FS_TYPE_PIPE))
        return 0;
    if(file->fsinfo.mount_pid <= 0)
        return 0;
    return vfs_driver_dup_now(file->fsinfo.mount_pid, from_pid, from_fd,
            dup_pid, dup_fd, file);
}

/* ---- directory kids lazy loading ---- */

/* caller must hold _vfs_lock (read or write) */
static inline bool vfs_node_kids_loaded(vfs_node_t* node) {
    if(node == NULL || !FS_IS_TYPE(node->fsinfo.type, FS_TYPE_DIR))
        return true;
    return (node->fsinfo.state & FS_STATE_KIDS_LOADED) != 0;
}

/* caller must hold _vfs_lock (write) */
static inline void vfs_set_kids_loaded(vfs_node_t* node) {
    if(node != NULL && FS_IS_TYPE(node->fsinfo.type, FS_TYPE_DIR))
        node->fsinfo.state |= FS_STATE_KIDS_LOADED;
}

/* caller must hold _vfs_lock (write) */
static int32_t vfs_commit_kids_to_node(vfs_node_t* father, const fsinfo_t* infos, uint32_t num) {
    if(father == NULL || !FS_IS_TYPE(father->fsinfo.type, FS_TYPE_DIR))
        return -1;

    for(uint32_t i = 0; i < num; i++) {
        fsinfo_t info;
        vfs_node_t* node;

        if(vfs_find_kid_raw(father, infos[i].name) != NULL)
            continue;

        node = vfsd_new_node();
        if(node == NULL)
            continue;

        memcpy(&info, &infos[i], sizeof(fsinfo_t));
        info.node = vfs_get_node_id(node);
        info.mount_pid = -1;
        memcpy(&node->fsinfo, &info, sizeof(fsinfo_t));
        vfs_add_node(0, father, node);
    }
    return 0;
}

/*
 * Runs on the async driver worker thread ONLY: outbound IPC to a mount
 * driver must never happen while _vfs_lock is held (see lock rules in
 * vfsd.h).
 */
static int32_t vfs_fetch_kids_from_driver(int32_t mount_pid, const fsinfo_t* info,
        uint32_t* num, fsinfo_t** infos) {
    if(num == NULL || infos == NULL || info == NULL || mount_pid <= 0)
        return -1;
    *num = 0;
    *infos = NULL;

    proto_t in, out;
    PF->format(&in, "i,m", info->node, info, sizeof(fsinfo_t));
    PF->init(&out);
    int32_t res = ipc_call(mount_pid, FS_CMD_KIDS, &in, &out);
    PF->clear(&in);
    if(res != 0) {
        PF->clear(&out);
        return -1;
    }

    *num = (uint32_t)proto_read_int(&out);
    int32_t sz = 0;
    fsinfo_t* out_infos = NULL;
    if(*num > 0) {
        fsinfo_t* src = (fsinfo_t*)proto_read(&out, &sz);
        if(src == NULL || sz < (int32_t)(sizeof(fsinfo_t) * (*num))) {
            PF->clear(&out);
            return -1;
        }

        out_infos = (fsinfo_t*)malloc(sizeof(fsinfo_t) * (*num));
        if(out_infos == NULL) {
            PF->clear(&out);
            return -1;
        }
        memcpy(out_infos, src, sizeof(fsinfo_t) * (*num));
    }

    PF->clear(&out);
    *infos = out_infos;
    return 0;
}

static void free_driver_kids_job(driver_kids_job_t* job) {
    if(job == NULL)
        return;
    if(job->infos != NULL)
        free(job->infos);
    free(job);
}

/* caller must hold _vfs_lock (write) */
static void vfs_apply_driver_kids_job_locked(driver_kids_job_t* job) {
    vfs_node_t* father;

    if(job == NULL)
        return;
    father = vfs_get_node_by_id(job->father_node_id);
    if(father == NULL)
        return;

    father->kids_loading = 0;
    if(job->res != 0)
        return;
    if(!FS_IS_TYPE(father->fsinfo.type, FS_TYPE_DIR))
        return;
    if(get_mount_pid(father) != job->mount_pid)
        return;

    (void)vfs_commit_kids_to_node(father, job->infos, job->num);
    vfs_set_kids_loaded(father);
}

/*
 * Must be called with NO _vfs_lock held: applying a result takes the lock
 * itself, and callers may loop here while other workers hold it.
 */
void vfs_drain_driver_kids_results(void) {
    while(true) {
        driver_kids_job_t* job;

        pthread_mutex_lock(&_driver_kids_results_lock);
        job = (driver_kids_job_t*)queue_pop(&_driver_kids_results);
        pthread_mutex_unlock(&_driver_kids_results_lock);
        if(job == NULL)
            break;

        pthread_rwlock_wrlock(&_vfs_lock);
        vfs_apply_driver_kids_job_locked(job);
        pthread_rwlock_unlock(&_vfs_lock);
        free_driver_kids_job(job);
    }
}

/* caller must hold _vfs_lock (write) - it stamps father->kids_loading */
static bool queue_driver_kids_job(vfs_node_t* father, int32_t mount_pid) {
    driver_kids_job_t* job;
    pthread_t tid;

    if(father == NULL || mount_pid <= 0)
        return false;
    if(_driver_async_worker.started == 0)
        return false;
    job = (driver_kids_job_t*)calloc(1, sizeof(driver_kids_job_t));
    if(job == NULL)
        return false;

    job->job_type = DRIVER_ASYNC_JOB_KIDS;
    job->mount_pid = mount_pid;
    job->father_node_id = vfs_get_node_id(father);
    vfs_fill_node_fsinfo(father, &job->father_info);

    pthread_mutex_lock(&_driver_async_worker.lock);
    queue_push(&_driver_async_worker.jobs, job);
    tid = _driver_async_worker.thread;
    pthread_mutex_unlock(&_driver_async_worker.lock);
    if(tid != 0)
        proc_wakeup_by((int32_t)tid, VFSD_WAKE_TOKEN_DRIVER_ASYNC);

    father->kids_loading = 1;
    return true;
}

static void driver_async_process_kids_job(driver_kids_job_t* job) {
    if(job == NULL)
        return;

    job->res = vfs_fetch_kids_from_driver(job->mount_pid, &job->father_info,
            &job->num, &job->infos);

    pthread_mutex_lock(&_driver_kids_results_lock);
    queue_push(&_driver_kids_results, job);
    pthread_mutex_unlock(&_driver_kids_results_lock);
}

#define KIDS_LOAD_WAIT_BUDGET_US   2000000U
#define KIDS_LOAD_WAIT_STEP_MIN_US 500U
#define KIDS_LOAD_WAIT_STEP_MAX_US 5000U

/*
 * Make sure a directory's kid list is loaded from its mount driver.
 * Called with NO _vfs_lock held: the wait below sleeps and the result
 * drain takes the lock itself. The node is re-validated by id after every
 * unlock, so a concurrent umount/del of the directory only ends the load
 * early, never dereferences a freed node.
 */
void vfs_ensure_kids_loaded(uint32_t node_id) {
    uint32_t waited = 0;
    uint32_t step = KIDS_LOAD_WAIT_STEP_MIN_US;

    if(node_id == 0)
        return;

    while(true) {
        bool loading = false;

        pthread_rwlock_wrlock(&_vfs_lock);
        vfs_node_t* father = vfs_get_node_by_id(node_id);
        if(father == NULL || vfs_node_kids_loaded(father)) {
            pthread_rwlock_unlock(&_vfs_lock);
            return;
        }

        int32_t mount_pid = get_mount_pid(father);
        if(mount_pid <= 0) {
            vfs_set_kids_loaded(father);
            father->kids_loading = 0;
            pthread_rwlock_unlock(&_vfs_lock);
            return;
        }

        if(!father->kids_loading) {
            if(!queue_driver_kids_job(father, mount_pid)) {
                /* worker not available: leave unloaded, next access retries */
                pthread_rwlock_unlock(&_vfs_lock);
                return;
            }
        }
        loading = (father->kids_loading != 0);
        pthread_rwlock_unlock(&_vfs_lock);
        if(!loading)
            return;

        /* apply any finished jobs OUTSIDE the lock, then re-check */
        vfs_drain_driver_kids_results();

        pthread_rwlock_rdlock(&_vfs_lock);
        father = vfs_get_node_by_id(node_id);
        bool done = (father == NULL) ||
            vfs_node_kids_loaded(father) || !father->kids_loading;
        pthread_rwlock_unlock(&_vfs_lock);
        if(done)
            return;

        if(waited >= KIDS_LOAD_WAIT_BUDGET_US) {
            /*
             * The original job keeps running on the shared async worker, but do not pin
             * this directory in a permanently "loading" state. Clearing the flag
             * lets the next access requeue and retry instead of timing out behind
             * an old stuck request forever.
             */
            pthread_rwlock_wrlock(&_vfs_lock);
            father = vfs_get_node_by_id(node_id);
            if(father != NULL)
                father->kids_loading = 0;
            pthread_rwlock_unlock(&_vfs_lock);
            return;
        }

        proc_usleep(step);
        waited += step;
        step *= 2;
        if(step > KIDS_LOAD_WAIT_STEP_MAX_US)
            step = KIDS_LOAD_WAIT_STEP_MAX_US;
    }
}
