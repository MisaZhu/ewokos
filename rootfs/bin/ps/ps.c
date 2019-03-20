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
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg != NULL && arg[0] == '-') {
		if(strchr(arg, 'a') != NULL) {
			owner = false;
		}
	}
	
	int num = 0;
	proc_info_t* procs = (proc_info_t*)syscall2(SYSCALL_GET_PROCS, (int)&num, owner);
	if(procs != NULL) {
		printf("--------------------------------------------------------------\n");
		for(int i=0; i<num; i++) {
			printf("%s\tpid:%d, father:%d, owner:%d, heap_size: %d\n", 
				procs[i].cmd,
				procs[i].pid,
				procs[i].father_pid,
				procs[i].owner,
				procs[i].heap_size);
		}
		printf("\n");
		free(procs);
	}
	exit(0);
}
