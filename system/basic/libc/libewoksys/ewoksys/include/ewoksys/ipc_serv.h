#ifndef IPC_SERV_H
#define IPC_SERV_H

#include <ewoksys/proto.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ipc_handled_t)(void* p);

#define IPC_SERV_VFS      "ipc_serv.vfs"
#define IPC_SERV_PROC     "ipc_serv.proc"

int ipc_serv_reg(const char* ipc_serv_id);
int ipc_serv_unreg(const char* ipc_serv_id);
int ipc_serv_get(const char* ipc_serv_id);

typedef void (*ipc_serv_handle_t)(int from_pid, int cmd, proto_t* in, proto_t* out, void* p);
int ipc_serv_run(ipc_serv_handle_t handle,
    ipc_handled_t handled,
    void* p,
    int flags);

#ifdef __cplusplus 
}
#endif

#endif
