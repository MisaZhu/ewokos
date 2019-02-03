#ifndef PROC_INFO_H
#define PROC_INFO_H

#include <types.h>

#define CMD_MAX 128

typedef struct {
	int32_t pid; 
	int32_t fatherPid;
	int32_t owner; 
	uint32_t heapSize;

	char cmd[CMD_MAX];
} ProcInfoT;
	
#endif
