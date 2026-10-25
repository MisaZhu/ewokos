#ifndef PROC_IPC_H
#define PROC_IPC_H

#include <proto.h>
#include <kernel/context.h>
#include <queue.h>
#include <stddef.h>

struct st_proc;

/*
 * Max in-flight IPC requests a server can hold (ipc_server_t.tasks[]).
 * Keep this SMALL: ipc_task_t embeds a proto_t with a 512-byte inline
 * buffer, and tasks[IPC_CTX_MAX] is inline in proc_space_t, which is
 * kmalloc'd for EVERY proc. At 64 each proc_space_t grew to ~40KB (~37KB
 * just for tasks[]), which exhausts the ~4MB kmalloc arena on small-RAM
 * boards -- the same class as the Miyoo/picocalc post-logo boot stall.
 * 16 keeps the per-proc footprint near the original while still letting a
 * multi_task server serve many connections concurrently: blocked/parked
 * clients hold NO task slot, so a server's connection ceiling is set by its
 * own resource tables (e.g. netd's TCP_PCB_SIZE/SOCKS_MAX), not by this.
 */
#define IPC_CTX_MAX 16
/*
 * Worker pool of a multi_task server (ipc_server_t.pool): the pool starts
 * EMPTY - no persistent minimum of worker threads is kept. Workers are
 * spawned on demand as requests arrive, each pinned to the requesting
 * client's core, and parked (BLOCKed) between requests for reuse. The pool
 * has no fixed hard cap: it is sized by the proc's own thread limit
 * (_kernel_config.max_task_per_proc, see pool_num). When the proc runs out
 * of thread slots the requesting client blocks inside the kernel until a
 * worker parks again.
 */

/*
 * Seconds a pool member may sit idle (parked, no request) before it
 * terminates itself to release its proc slot and thread stack. Every
 * parked member is eligible, so an idle pool drains back to empty.
 */
#define IPC_TASK_SELF_QUIT_TIMEOUT  3 //seconds

enum {
	SIG_STATE_IDLE = 0,
	SIG_STATE_BUSY
};

typedef struct {
	uint32_t  uid;
	uint32_t  state;
	proto_t   data;
} ipc_res_t;

typedef	struct {
	uint32_t  uid;
	uint32_t  counter;
	uint32_t  state;
	proto_t   arg_ret;
	int32_t   client_pid;
	uint32_t  client_uuid;
	uint8_t   client_intr; //client issued this call from an interrupt handler
	int32_t   call_id;
	int32_t   handler_pid;  //multi_task mode: worker thread serving this task
	uint32_t  handler_uuid; //uuid of the worker thread (pid slot reuse guard)
} ipc_task_t;

typedef struct ipc_queue_item {
	struct st_proc* owner;
	int32_t         pid;
	uint32_t        uuid;
	uint8_t         queued;
	struct ipc_queue_item* next;
	struct ipc_queue_item* prev;
} ipc_queue_item_t;

typedef struct {
	int32_t  pid;  //0 = empty slot
	uint32_t uuid; //uuid of the worker (pid slot reuse guard)
	uint32_t idle_sec; //uptime_sec when the worker parked (0 = not idle)
	uint8_t  quit; //idle sweep asked this member to terminate itself
} ipc_pool_worker_t;

typedef struct {
	int32_t       lock;  // per-server spinlock for SMP protection
	bool          disabled;
	bool          multi_task;
	ewokos_addr_t entry;
	uint32_t      flags;
	ewokos_addr_t extra_data;
	ipc_task_t    tasks[IPC_CTX_MAX];
	uint8_t       task_head;
	uint8_t       task_tail;
	uint8_t       task_num;
	/*
	 * Persistent worker threads (multi_task). Dynamically sized by the
	 * proc's own thread limit (_kernel_config.max_task_per_proc); pool_num
	 * holds the number of allocated slots. NULL/0 for non multi_task procs.
	 */
	ipc_pool_worker_t* pool;
	uint32_t      pool_num;

    bool          do_switch;
	uint8_t       restore_pending;
	ewokos_addr_t stack; //mapped stack page

	ipc_res_t     saved_ipc_res;
	saved_state_t saved_state;
	ipc_queue_item_t* wait_head;
	ipc_queue_item_t* wait_tail;
} ipc_server_t;

#ifdef KERNEL_SMP
extern void mcore_lock(int32_t* v);
extern void mcore_unlock(int32_t* v);
#endif

static inline void proc_ipc_server_lock(ipc_server_t* server) {
#ifdef KERNEL_SMP
	if(server != NULL)
		mcore_lock(&server->lock);
#else
	(void)server;
#endif
}

static inline void proc_ipc_server_unlock(ipc_server_t* server) {
#ifdef KERNEL_SMP
	if(server != NULL)
		mcore_unlock(&server->lock);
#else
	(void)server;
#endif
}


extern int32_t     proc_ipc_setup(context_t* ctx, ewokos_addr_t entry, ewokos_addr_t extra, uint32_t flags);
extern int32_t     proc_ipc_do_task(context_t* ctx, struct st_proc* proc, uint32_t core);
extern ipc_task_t* proc_ipc_req(struct st_proc* serv_proc, struct st_proc* client_proc, int32_t call_id, proto_t* arg);
extern uint32_t    proc_ipc_fetch(struct st_proc* serv_proc);
extern ipc_task_t* proc_ipc_get_task(struct st_proc* serv_proc);
extern uint32_t    proc_ipc_task_count(struct st_proc* serv_proc);
extern ipc_task_t* proc_ipc_find_task(struct st_proc* serv_proc, uint32_t uid);
extern ipc_task_t* proc_ipc_current_task(struct st_proc* proc);
extern ipc_task_t* proc_ipc_serving_task(struct st_proc* proc, uint32_t uid);
extern struct st_proc* proc_ipc_pool_spawn(struct st_proc* serv_proc, uint32_t core);
extern void        proc_ipc_pool_park(context_t* ctx, struct st_proc* worker, struct st_proc* serv_proc, struct st_proc* wake_client);
extern void        proc_ipc_pool_shrink(struct st_proc* serv_proc);
extern void        proc_ipc_task_abort(struct st_proc* serv_proc, ipc_task_t* ipc);
extern struct st_proc* proc_ipc_finish_task(struct st_proc* serv_proc, struct st_proc* worker, ipc_task_t* ipc, bool* wake_client);
extern void        proc_ipc_close(struct st_proc* serv_proc, ipc_task_t* ipc);
extern void        proc_ipc_clear(struct st_proc* serv_proc);
extern int32_t     proc_ipc_wait(context_t* ctx, struct st_proc* serv_proc, struct st_proc* proc);
extern void        proc_ipc_cancel_wait(struct st_proc* proc);
extern struct st_proc*  proc_ipc_wakeup(struct st_proc* serv_proc);
extern void        proc_ipc_wakeup_all(struct st_proc* serv_proc);

/*
 * Syscall-level ipc entry points (implementations of SYS_IPC_*), kept here
 * so svc.c stays a thin dispatcher.
 */
extern void        proc_ipc_call(context_t* ctx, int32_t serv_pid, int32_t call_id, proto_t* arg);
extern void        proc_ipc_get_return(context_t* ctx, int32_t serv_pid, uint32_t uid, proto_t* data);
extern int32_t     proc_ipc_get_arg(uint32_t uid, int32_t* ipc_info, proto_t* arg);
extern void        proc_ipc_set_return(uint32_t uid, proto_t* data);
extern void        proc_ipc_end(context_t* ctx);
extern int32_t     proc_ipc_disable(void);
extern void        proc_ipc_enable(void);
extern bool        proc_ipc_sync_serving(struct st_proc* proc);

#endif
