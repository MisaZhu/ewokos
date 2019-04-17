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
	uint32_t sec;
	syscall3(SYSCALL_SYSTEM_CMD, 4, (int32_t)&sec, 0);

	proc_info_t* procs = (proc_info_t*)syscall2(SYSCALL_SYSTEM_CMD, 2, (int)&num);
	if(procs != NULL) {
		for(int i=0; i<num; i++) {
			printf("%16s pid:%4d father:%4d owner:%4d state: %4d cpu_time: %8d heap_size: %d\n", 
				procs[i].cmd,
				procs[i].pid,
				procs[i].father_pid,
				procs[i].owner,
				procs[i].state,
				sec - procs[i].start_sec,
				procs[i].heap_size);
		}
		free(procs);
	}

	printf("--------------------------------------------------------------\n");
	if(fr_mem > 1024)
		printf("memory: total %d MB, free %d KB (%d MB)\n", t_mem, fr_mem, fr_mem/1024);
	else
		printf("memory: total %d MB, free %d KB\n", t_mem, fr_mem);
	printf("processes: %d\n", num);
	return 0;
}
