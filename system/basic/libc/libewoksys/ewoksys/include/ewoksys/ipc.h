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

key_t ftok(const char* fname, int proj_id);

int      ipc_call(int to_pid, int call_id, const proto_t* ipkg, proto_t* opkg);
int      ipc_call_wait(int to_pid, int call_id, const proto_t* ipkg);

void     ipc_ready(void);
int      ipc_wait_ready(int pid);

int      ipc_disable(void);
void     ipc_enable(void);

#ifdef __cplusplus 
}
#endif

#endif
