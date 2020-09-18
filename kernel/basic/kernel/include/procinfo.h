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
	PROC_TYPE_IPC
};

#define IPC_NONBLOCK    0x01

typedef struct {
	int32_t  type; 
	int32_t  pid; 
	int32_t  father_pid;
	int32_t  owner; 
	int32_t  state; 
	int32_t  block_by;
	int32_t  wait_for;
	bool     ipc_busy;
	uint32_t start_sec;
	char     cmd[PROC_INFO_CMD_MAX];
} procinfo_t;
	
#endif
