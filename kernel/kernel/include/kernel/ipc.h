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
	int32_t   call_id;
} ipc_task_t;


typedef struct {
	bool          disabled;
	ewokos_addr_t entry;
	uint32_t      flags;
	ewokos_addr_t extra_data;
	ipc_task_t    ctask; //current_task

    bool          do_switch;
	uint32_t      stack; //mapped stack page

	ipc_res_t     saved_ipc_res;
	saved_state_t saved_state;
} ipc_server_t;

extern int32_t     proc_ipc_setup(context_t* ctx, ewokos_addr_t entry, uint32_t extra, uint32_t flags);
extern int32_t     proc_ipc_do_task(context_t* ctx, struct st_proc* proc, uint32_t core);
extern ipc_task_t* proc_ipc_req(struct st_proc* serv_proc, struct st_proc* client_proc, int32_t call_id, proto_t* arg);
extern uint32_t    proc_ipc_fetch(struct st_proc* serv_proc);
extern ipc_task_t* proc_ipc_get_task(struct st_proc* serv_proc);
extern void        proc_ipc_close(struct st_proc* serv_proc, ipc_task_t* ipc); 
extern void        proc_ipc_clear(struct st_proc* serv_proc);

#endif
