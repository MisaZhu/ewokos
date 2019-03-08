#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <kstring.h>
#include <cmain.h>

void _start()
{
	bool owner = true; /*only show process with same owner of the current proc*/
	initCMainArg();
	const char* arg = readCMainArg();
	arg = readCMainArg();
	if(arg != NULL && arg[0] == '-') {
		if(strchr(arg, 'a') != NULL) {
			owner = false;
		}
	}
	
	int num = 0;
	ProcInfoT* procs = (ProcInfoT*)syscall2(SYSCALL_GET_PROCS, (int)&num, owner);
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
