#ifndef PROC_IPC_H
#define PROC_IPC_H

#include <kernel/proc.h>
#include <proto.h>
#include <stdbool.h>

typedef	struct {
	proto_t data;
	uint32_t state;
	uint32_t proc_state;
	int32_t from_pid;
	int32_t call_id;
	context_t ctx;
} ipc_t;

extern int32_t     proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra, bool nonblock);
extern int32_t     proc_ipc_call(context_t* ctx, proc_t* proc, ipc_t* ipc);

#endif
