/*
 * vfsd - virtual file system server (IPC_MULTI_TASK).
 *
 * Common types, globals and internal API shared by the split translation
 * units:
 *   node.c           node alloc/hash, tree ops, path resolving
 *   mount.c          mount table, mount/umount
 *   fd.c             fd table, open/close/dup/dup2, fsinfo snapshots
 *   waitq.c          wait queues and node event wakeup
 *   driver_async.c   async driver worker (close/dup/kids jobs), kids lazy load
 *   proc.c           process lifecycle (clone/exit/zombie, fd slot tracking)
 *   pipe.c           pipe open/read/write, shm pipe lifecycle
 *   handlers.c       node/mount/query IPC handlers + main dispatch
 *   proc_handlers.c  proc/block/poll IPC handlers
 *   vfsd.c           init + main()
 */
#ifndef _VFSD_H_
#define _VFSD_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/shm.h>
#include <pthread.h>
#include <ewoksys/ipc.h>
#include <ewoksys/ipc_serv.h>
#include <ewoksys/klog.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/proc.h>
#include <ewoksys/mstr.h>
#include <ewoksys/buffer.h>
#include <ewoksys/proto.h>
#include <ewoksys/fsinfo.h>
#include <ewoksys/sys.h>
#include <ewoksys/syscall.h>
#include <ewoksys/hashmap.h>
#include <ewoksys/queue.h>
#include <ewoksys/shm_pipe.h>
#include <procinfo.h>
#include <sysinfo.h>

typedef struct vfs_node {
    struct vfs_node* father;
    struct vfs_node* first_kid; /*first child*/
    struct vfs_node* last_kid; /*last child*/
    struct vfs_node* next; /*next brother*/
    struct vfs_node* prev; /*prev brother*/
    void* data_ptr;
    shm_pipe_t* shm_ring;     /* shared-memory pipe ring (NULL if not allocated) */
    uint32_t node_id;
    uint32_t kids_num;

    fsinfo_t fsinfo;

    int32_t mount_id;
    uint8_t pending_umount;
    uint8_t kids_loading;
    uint32_t refs;
    uint32_t refs_w;
    uint32_t events; //events for poll or select
    queue_t read_wait_queue;
    queue_t write_wait_queue;
} vfs_node_t;

typedef struct {
    vfs_node_t *node;
    uint32_t flags;
    /*
     * Tracks whether this fd owns a driver-side reference that must be
     * returned with FS_CMD_CLOSE when the fd dies without an explicit
     * user-space close. Set by vfsd_open() (open/accept create driver
     * state) and by do_vfs_proc_clone() (fork sends FS_CMD_DUP per
     * inherited fd); cleared by vfsd_dup()/vfsd_dup2() because same-process
     * duplication never notifies the driver.
     */
    uint32_t driver_ref;
    fsinfo_t fsinfo;
} file_t;

typedef struct {
    int32_t pid;
    uint32_t uuid;
    queue_item_t item;
    queue_t* queue;
    uint32_t node_id;
} wait_entry_t;

typedef struct {
    uint32_t uuid;
    uint32_t state;
    int32_t owner_pid;
    wait_entry_t read_waiter;
    wait_entry_t write_waiter;
    file_t fds[MAX_OPEN_FILE_PER_PROC];
} proc_fds_t;

typedef struct {
    int32_t pid;
    uint32_t uuid;
} zombie_task_t;

typedef enum {
    DRIVER_ASYNC_JOB_NONE = 0,
    DRIVER_ASYNC_JOB_CLOSE = 1,
    DRIVER_ASYNC_JOB_DUP = 2,
    DRIVER_ASYNC_JOB_KIDS = 3,
} driver_async_job_type_t;

typedef struct {
    driver_async_job_type_t job_type;
    int32_t pid;
    int32_t owner_pid;
    int32_t fd;
    file_t file;
} driver_close_task_t;

typedef struct clone_dup_ctx {
    pthread_mutex_t lock;
    int pending;
    pthread_t waiter;
    /*
     * Shared between the main IPC context and dup worker threads: the
     * bounded wait below can give up and return while a worker still
     * holds job->ctx, so the ctx must be heap-allocated and refcounted
     * instead of living on the caller's stack.
     */
    int refs;
} clone_dup_ctx_t;

typedef struct {
    driver_async_job_type_t job_type;
    int32_t mount_pid;
    int32_t from_pid;
    int32_t from_fd;
    int32_t dup_pid;
    int32_t dup_fd;
    uint32_t dup_uuid;
    file_t file;
    /*
     * NULL => fire-and-forget job used by VFS_PROC_CLONE so vfsd never
     * waits for a third-party driver while serving IPC.
     */
    clone_dup_ctx_t* ctx;
} driver_dup_job_t;

typedef struct {
    pthread_t thread;
    pthread_mutex_t lock;
    queue_t jobs;
    uint8_t started;
} driver_async_worker_t;

typedef struct {
    driver_async_job_type_t job_type;
    int32_t mount_pid;
    uint32_t father_node_id;
    fsinfo_t father_info;
    int32_t res;
    uint32_t num;
    fsinfo_t* infos;
} driver_kids_job_t;

#define VFSD_WAKE_TOKEN_DRIVER_ASYNC 0x56464153U
#define VFSD_WAKE_TOKEN_CLONE_DUP    0x56464450U

extern vfs_node_t* _vfs_root;
extern mount_t _vfs_mounts[FS_MOUNT_MAX];
extern map_t  _nodes_hash;
extern proc_fds_t* _proc_fds_table;
extern uint32_t    _max_proc_table_num;
extern queue_t     _zombie_tasks;
extern driver_async_worker_t _driver_async_worker;
extern queue_t _driver_kids_results;
extern pthread_mutex_t _driver_kids_results_lock;

/*
 * Concurrency model: vfsd runs with IPC_MULTI_TASK, so every incoming VFS
 * request is served by its own kernel-spawned worker thread and ALL state
 * above (node tree/hash, mount table, per-proc fd table, wait queues,
 * zombie queue) is shared between concurrently running handlers.
 *
 * _vfs_lock (rwlock) guards all of that shared state:
 *  - read-only handlers (get by name/node/fd, kids, mount-by-id, poll
 *    events query) take it shared;
 *  - every mutating handler takes it exclusive.
 *
 * Hard rules while holding _vfs_lock:
 *  - NEVER sleep, proc_usleep(), proc_block_by() or wait on a driver;
 *  - NEVER issue IPC to a mount driver (FS_CMD_*): the driver can call
 *    back into vfsd (vfs_wakeup/vfs_new_nodes) and that callback lands in
 *    a DIFFERENT vfsd worker which would then deadlock on this lock while
 *    we wait for the driver's reply;
 *  - kernel syscalls (proc_info/proc_get_uuid/proc_wakeup_by/shmget/...)
 *    are fine: they never block on another user-space server.
 *
 * Nesting order (outer -> inner): _vfs_lock -> _driver_async_worker.lock,
 * _driver_kids_results_lock, clone_dup_ctx.lock. The async driver worker
 * and the dup/kids completion paths take those inner locks WITHOUT ever
 * holding _vfs_lock at the same time, except driver_dup_job_still_valid()
 * which takes _vfs_lock alone.
 *
 * Outbound driver IPC (FS_CMD_DUP waits, fallback dups) always runs OUTSIDE
 * _vfs_lock; node access across an unlock is re-validated by node_id.
 */
extern pthread_rwlock_t _vfs_lock;

/* ---- node.c ---- */
extern uint32_t vfs_get_node_id(vfs_node_t* node);
extern vfs_node_t* vfsd_new_node(void); /* caller must hold _vfs_lock (write) */
extern vfs_node_t* vfs_get_node_by_id(uint32_t node_id);
extern int32_t vfs_add_node(int32_t pid, vfs_node_t* father, vfs_node_t* node);
extern void vfs_remove(int32_t pid, vfs_node_t* node);
extern int32_t vfsd_del_node(vfs_node_t* node);
extern int32_t set_node_info(int32_t pid, vfs_node_t* node, fsinfo_t* info);
extern void vfsd_fullname(vfs_node_t* node, char* out, uint32_t out_sz);
extern vfs_node_t* vfs_find_kid_raw(vfs_node_t* father, const char* name);
extern bool vfs_resolve_path(const char* name, uint32_t* node_id_out);
extern int vfsd_check_access(int pid, fsinfo_t* info, int mode);

/* ---- mount.c ---- */
extern int32_t get_mount_pid(vfs_node_t* node);
extern int32_t vfsd_get_mount_by_id(int32_t id, mount_t* mount);
extern int32_t vfsd_mount(int32_t pid, vfs_node_t* org, vfs_node_t* node, const char* desc);
extern void vfs_try_finish_umount(vfs_node_t* node);
extern void vfsd_umount(int32_t pid, vfs_node_t* node);

/* ---- fd.c ---- */
extern int32_t vfs_fd_owner_pid(int32_t pid);
extern file_t* vfs_get_file(int32_t pid, int32_t fd);
extern file_t* vfs_check_fd(int32_t pid, int32_t fd);
extern int32_t vfsd_open(int32_t pid, vfs_node_t* node, int32_t flags);
extern vfs_node_t* vfs_open_announimous(int32_t pid, vfs_node_t* node);
extern vfs_node_t* vfsd_get_by_fd(int32_t pid, int32_t fd);
extern void vfsd_close(int32_t pid, int32_t fd);
extern vfs_node_t* vfsd_dup(int32_t pid, int32_t from, int32_t *ret);
extern vfs_node_t* vfsd_dup2(int32_t pid, int32_t from, int32_t to);
extern void vfs_fill_node_fsinfo(vfs_node_t* node, fsinfo_t* out);
extern int32_t vfs_fill_file_fsinfo(file_t* file, fsinfo_t* out);
extern fsinfo_t* vfs_get_kids(uint32_t node_id, uint32_t* num);

/* ---- waitq.c ---- */
extern wait_entry_t* get_wait_entry(int32_t pid, bool wr);
extern void wait_queue_remove_entry(wait_entry_t* waiter);
extern wait_entry_t* wait_queue_pop(queue_t* q);
extern void wakeup_proc(wait_entry_t* waiter, vfs_node_t* node, int32_t events);
extern void enqueue_waiter(queue_t* q, int32_t pid, uint32_t uuid, bool wr, uint32_t node_id);
extern void do_node_wakeup(vfs_node_t* node, int events);
extern int32_t get_tracked_owner_pid(int32_t pid);

/* ---- driver_async.c ---- */
extern void start_driver_async_worker(void);
extern void enqueue_driver_close_task(driver_close_task_t* task);
extern int vfs_driver_dup_now(int32_t mount_pid, int32_t from_pid, int32_t from_fd,
        int32_t dup_pid, int32_t dup_fd, const file_t* file);
extern void vfs_driver_dup(int32_t from_pid, int32_t from_fd,
        int32_t dup_pid, int32_t dup_fd, file_t* file);
extern bool queue_driver_dup_job(clone_dup_ctx_t* ctx, int32_t mount_pid,
        int32_t from_pid, int32_t from_fd,
        int32_t dup_pid, int32_t dup_fd, file_t* file);
extern clone_dup_ctx_t* clone_dup_ctx_create(void);
extern void clone_dup_ctx_unref(clone_dup_ctx_t* ctx);
extern void clone_dup_ctx_wait(clone_dup_ctx_t* ctx, int32_t mount_pid);
extern void vfs_drain_driver_kids_results(void);
extern void vfs_ensure_kids_loaded(uint32_t node_id);

/* ---- proc.c ---- */
extern void vfs_track_task_slot(int32_t pid);
extern void clear_pending_zombies(void* p);
extern void do_vfs_proc_exit(int32_t pid, proto_t* in);
extern void remove_zombie_task(int32_t pid, uint32_t uuid);
extern void clear_zombie(int32_t cpid);

/* ---- pipe.c ---- */
extern void sync_pipe_poll_events(vfs_node_t* node);
extern void proc_file_close(int pid, int fd, file_t* file);
extern void do_vfs_pipe_open(int32_t pid, proto_t* out);
extern void do_vfs_pipe_write(int pid, proto_t* in, proto_t* out);
extern void do_vfs_pipe_read(int pid, proto_t* in, proto_t* out);

/* ---- proc_handlers.c ---- */
extern void do_vfs_proc_clone(int32_t pid, proto_t* in);
extern void do_vfs_block(int32_t pid, proto_t* in);
extern void do_vfs_wakeup(int32_t pid, proto_t* in);
extern void do_vfs_unblock(int32_t pid, proto_t* in);
extern void do_vfs_get_poll_events(int32_t pid, proto_t* in, proto_t* out);
extern void do_vfs_clear_poll_events(int32_t pid, proto_t* in, proto_t* out);

/* ---- handlers.c ---- */
extern void handle(int pid, int cmd, proto_t* in, proto_t* out, void* p);

#endif
