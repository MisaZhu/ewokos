#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <string.h>

void _start()
{
	int num = 0;
	ProcInfoT* procs = (ProcInfoT*)syscall1(SYSCALL_GET_PROCS, (int)&num);
	if(procs != NULL) {
		printf("--------------------------------------------------------------\n");
		for(int i=0; i<num; i++) {
			printf("%s\tpid:%d, father:%d, owner:%d, heapSize: %d\n", 
				procs[i].cmd,
				procs[i].pid,
				procs[i].fatherPid,
				procs[i].owner,
				procs[i].heapSize);
		}
		printf("\n");
		free(procs);
	}
	exit(0);
}
