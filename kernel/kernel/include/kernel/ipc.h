#ifndef PROC_IPC_H
#define PROC_IPC_H

#include <proto.h>
#include <kernel/context.h>

struct st_proc;
typedef	struct {
	uint32_t  uid;
	uint64_t  usec;
	uint32_t  state;
	proto_t   data;
	int32_t   client_pid;
	int32_t   call_id;
} ipc_task_t;

extern int32_t     proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra, uint32_t flags);
extern int32_t     proc_ipc_do_task(context_t* ctx, struct st_proc* proc, uint32_t core);
extern ipc_task_t* proc_ipc_req(struct st_proc* serv_proc, int32_t client_pid, int32_t call_id, proto_t* data);
extern uint32_t    proc_ipc_fetch(struct st_proc* serv_proc);
extern ipc_task_t* proc_ipc_get_task(struct st_proc* serv_proc);
extern void        proc_ipc_close(struct st_proc* serv_proc, ipc_task_t* ipc); 
extern void        proc_ipc_clear(struct st_proc* serv_proc);

#endif
