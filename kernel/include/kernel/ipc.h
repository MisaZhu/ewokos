#ifndef PROC_IPC_H
#define PROC_IPC_H

#include <proto.h>
#include <kernel/context.h>
#include <stdbool.h>

struct st_proc;

typedef	struct {
	proto_t data;
	uint32_t state;
	uint32_t proc_state;
	int32_t pid_client;
	int32_t pid_server;
	int32_t call_id;
	context_t ctx;
} ipc_t;

extern int32_t  proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra, uint32_t flags);
extern int32_t  proc_ipc_call(context_t* ctx, struct st_proc* proc, ipc_t* ipc);
extern ipc_t*   proc_ipc_req(struct st_proc* proc);
extern void     proc_ipc_close(ipc_t* ipc);

#endif
