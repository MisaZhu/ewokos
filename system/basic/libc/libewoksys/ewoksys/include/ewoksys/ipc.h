#ifndef IPC_H
#define IPC_H

#include <ewoksys/proto.h>
#include <ewoksys/ewokdef.h>
#include <procinfo.h>

#ifdef __cplusplus
extern "C" {
#endif


#define 	IPC_PRIVATE 0
#define 	IPC_CREAT   00001000 /* create if key is nonexistent */
#define 	IPC_EXCL    00002000 /* fail if key exists */
#define 	IPC_NOWAIT  00004000 /* return error on wait */
#define 	IPC_DIPC    00010000 /* make it distributed */
#define 	IPC_OWN     00020000 /* this machine is the DIPC owner */
#define 	IPC_RMID    0 /* remove resource */
#define 	IPC_SET     1 /* set ipc_perm options */
#define 	IPC_STAT    2 /* get ipc_perm options */
#define 	IPC_INFO    3 /* see ipcs */

typedef long int key_t;

typedef void (*ipc_handle_t)(uint32_t ipc_id, void* p);
typedef void (*ipc_handled_t)(void* p);

key_t ftok(const char* fname, int proj_id);

int      ipc_call(int to_pid, int call_id, const proto_t* ipkg, proto_t* opkg);
int      ipc_call_wait(int to_pid, int call_id, const proto_t* ipkg);

void     ipc_ready(void);
void     ipc_wait_ready(int pid);

int      ipc_disable(void);
void     ipc_enable(void);

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
