#ifndef PROC_IPC_H
#define PROC_IPC_H

#include <proto.h>
#include <kernel/context.h>
#include <queue.h>

struct st_proc;

#define IPC_CTX_MAX 8

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

    bool          do_switch;
	uint8_t       restore_pending;
	ewokos_addr_t stack; //mapped stack page

	ipc_res_t     saved_ipc_res;
	saved_state_t saved_state;
	ipc_queue_item_t* wait_head;
	ipc_queue_item_t* wait_tail;
} ipc_server_t;

extern int32_t     proc_ipc_setup(context_t* ctx, ewokos_addr_t entry, ewokos_addr_t extra, uint32_t flags);
extern int32_t     proc_ipc_do_task(context_t* ctx, struct st_proc* proc, uint32_t core);
extern ipc_task_t* proc_ipc_req(struct st_proc* serv_proc, struct st_proc* client_proc, int32_t call_id, proto_t* arg);
extern uint32_t    proc_ipc_fetch(struct st_proc* serv_proc);
extern ipc_task_t* proc_ipc_get_task(struct st_proc* serv_proc);
extern uint32_t    proc_ipc_task_count(struct st_proc* serv_proc);
extern ipc_task_t* proc_ipc_find_task(struct st_proc* serv_proc, uint32_t uid);
extern ipc_task_t* proc_ipc_current_task(struct st_proc* proc);
extern ipc_task_t* proc_ipc_serving_task(struct st_proc* proc, uint32_t uid);
extern int32_t     proc_ipc_spawn_worker(context_t* ctx, struct st_proc* serv_proc, ipc_task_t* ipc);
extern void        proc_ipc_task_abort(struct st_proc* serv_proc, ipc_task_t* ipc);
extern struct st_proc* proc_ipc_finish_task(struct st_proc* serv_proc, ipc_task_t* ipc, bool* wake_client);
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
