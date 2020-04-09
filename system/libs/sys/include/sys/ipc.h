#ifndef IPC_H
#define IPC_H

#include <proto.h>

int ipc_send(int to_pid, const proto_t* pkg, int32_t id);
int ipc_get(int* from_pid,  proto_t* pkg, int32_t id, uint8_t block);
int ipc_msg_call(int to_pid,  const proto_t* ipkg, proto_t* opkg);

typedef void (*ipc_msg_handle_t)(int from_pid, proto_t *in, void* p);
int ipc_server(ipc_msg_handle_t handle, void* p);

typedef void (*ipc_handle_t)(int from_pid, int call_id, void* p);
int ipc_call(int to_pid, int call_id, const proto_t* ipkg, proto_t* opkg);
int ipc_setup(ipc_handle_t handle, void* p);
int ipc_return(const proto_t* ipkg);
proto_t* ipc_get_arg(void);

#endif
