#ifndef PROC_INFO_H
#define PROC_INFO_H

#include <stdint.h>
#include <stdbool.h>

#define PROC_INFO_CMD_MAX 256
#define PROC_MAX 128

enum {
	UNUSED = 0,
	CREATED,
	SLEEPING,
	WAIT,
	BLOCK,
	READY,
	RUNNING,
	ZOMBIE
};

enum {
	PROC_TYPE_PROC = 0,
	PROC_TYPE_THREAD,
	PROC_TYPE_IPC,
	PROC_TYPE_VFORK
};

enum {
	IPC_IDLE = 0,
	IPC_BUSY,
	IPC_RETURN
};


#define IPC_NON_BLOCK        0x01
#define IPC_NON_RETURN       0x80000000
#define IPC_NON_RETURN_MASK  0x7fffffff

typedef struct {
	int32_t  type; 
	uint32_t  core;
	int32_t  pid; 
	int32_t  father_pid;
	int32_t  owner; 
	int32_t  state; 
	int32_t  block_by;
	int32_t  wait_for;
	int32_t  ipc_state;
	uint32_t start_sec;
	char     cmd[PROC_INFO_CMD_MAX];
} procinfo_t;
	
#endif
