#ifndef PROC_H
#define PROC_H

#include <stdint.h>

enum {
	PROC_CMD_SET_CWD = 0,
	PROC_CMD_GET_CWD,
	PROC_CMD_SET_ENV,
	PROC_CMD_GET_ENV,
	PROC_CMD_GET_ENVS,
	PROC_CMD_CLONE,
	PROC_CMD_EXIT
};

int proc_ping(int pid);
void proc_ready_ping(void);
void proc_wait_ready(int pid);

#endif
