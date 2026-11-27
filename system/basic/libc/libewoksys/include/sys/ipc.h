#ifndef SYS_IPC_H
#define SYS_IPC_H

#include <stdint.h>

typedef long int key_t;

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

key_t ftok(const char* fname, int proj_id);

#endif