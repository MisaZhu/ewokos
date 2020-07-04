#ifndef IPC_H
#define IPC_H

#include <sys/proto.h>
#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*ipc_handle_t)(int from_pid, int call_id, void* p);
int      ipc_call(int to_pid, int call_id, const proto_t* ipkg, proto_t* opkg);
int      ipc_setup(ipc_handle_t handle, void* p, bool prefork);
int      ipc_set_return(const proto_t* ipkg);
proto_t* ipc_get_arg(void);
void     ipc_end(void);
void     ipc_ready(void);

#define IPC_SERV_VFS      "ipc_serv.vfs"
#define IPC_SERV_PS2_KEYB "ipc_serv.ps2keyb"
#define IPC_SERV_LOCK     "ipc_serv.lock"
#define IPC_SERV_PROC     "ipc_serv.proc"

int ipc_serv_reg(const char* ipc_serv_id);
int ipc_serv_unreg(const char* ipc_serv_id);
int ipc_serv_get(const char* ipc_serv_id);

typedef void (*ipc_serv_handle_t)(int from_pid, int cmd, proto_t* in, proto_t* out, void* p);
int ipc_serv_run(ipc_serv_handle_t handle, void* p, bool prefork);

#ifdef __cplusplus 
}
#endif

#endif
