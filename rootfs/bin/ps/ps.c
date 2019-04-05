#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <kstring.h>
#include <cmain.h>

int main() {
	int num = 0;
	uint32_t fr_mem = (uint32_t)syscall2(SYSCALL_SYSTEM_CMD, 1, 0) / KB;
	uint32_t t_mem = (uint32_t)syscall2(SYSCALL_SYSTEM_CMD, 0, 0) / MB;
	printf("--------------------------------------------------------------\n");
	if(fr_mem > 1024)
		printf("memory: total %d MB, free %d KB (%d MB)\n", t_mem, fr_mem, fr_mem/1024);
	else
		printf("memory: total %d MB, free %d KB\n", t_mem, fr_mem);
	printf("--------------------------------------------------------------\n");

	proc_info_t* procs = (proc_info_t*)syscall2(SYSCALL_SYSTEM_CMD, 2, (int)&num);
	if(procs != NULL) {
		for(int i=0; i<num; i++) {
			printf("%s\tpid:%d, father:%d, owner:%d, state: %d heap_size: %d\n", 
				procs[i].cmd,
				procs[i].pid,
				procs[i].father_pid,
				procs[i].owner,
				procs[i].state,
				procs[i].heap_size);
		}
		printf("\n");
		free(procs);
	}
	return 0;
}
