#ifndef PROC_IPC_H
#define PROC_IPC_H

#include <proto.h>
#include <kernel/context.h>
#include <stdbool.h>

struct st_proc;
typedef	struct {
	uint32_t  uid;
	uint64_t  usec;
	uint32_t  state;
	proto_t   data;
	int32_t   client_pid;
	int32_t   call_id;
} ipc_task_t;

typedef	struct {
	context_t saved_ctx;
	uint32_t  saved_state;
	int32_t   saved_block_by;
	uint32_t  saved_block_event;
} ipc_context_t;

typedef struct {
	uint32_t  uid;
	uint32_t  state;
	proto_t   data;
} ipc_res_t;

extern int32_t     proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra, uint32_t flags);
extern void        proc_ipc_do_task(context_t* ctx, struct st_proc* proc);
extern ipc_task_t* proc_ipc_req(int32_t serv_pid, int32_t call_id, proto_t* data);
extern void        proc_ipc_close(ipc_task_t* ipc); 

#endif
