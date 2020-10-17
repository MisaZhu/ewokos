#ifndef PROC_IPC_H
#define PROC_IPC_H

#include <proto.h>
#include <kernel/context.h>
#include <stdbool.h>

struct st_proc;

typedef	struct {
	uint32_t state;
	proto_t  data;
	int32_t  server_pid;
	int32_t  client_pid;
	int32_t  call_id;
	bool     noret;
} ipc_t;

extern int32_t  proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra, uint32_t flags);
extern int32_t  proc_ipc_call(context_t* ctx, struct st_proc* proc, ipc_t* ipc);
extern ipc_t*   proc_ipc_req(struct st_proc* proc);
extern void     proc_ipc_close(ipc_t* ipc);

#endif
