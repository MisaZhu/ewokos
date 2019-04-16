#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <kstring.h>
#include <cmain.h>

#define ALIGN_LEN 16
static const char* align_cmd(const char* cmd) {
	static char ret[ALIGN_LEN+1];
	int32_t i = 0;

	while(i< ALIGN_LEN) {
		ret[i] = cmd[i];
		if(cmd[i] == 0)
			break;
		i++;
	}
	
	while(i< ALIGN_LEN) {
		ret[i++] = ' ';
	}
	ret[i] = 0;
	return ret;
}

int main() {
	int num = 0;
	uint32_t fr_mem = (uint32_t)syscall2(SYSCALL_SYSTEM_CMD, 1, 0) / KB;
	uint32_t t_mem = (uint32_t)syscall2(SYSCALL_SYSTEM_CMD, 0, 0) / MB;

	proc_info_t* procs = (proc_info_t*)syscall2(SYSCALL_SYSTEM_CMD, 2, (int)&num);
	if(procs != NULL) {
		for(int i=0; i<num; i++) {
			const char* cmd = align_cmd(procs[i].cmd);
			printf("%s pid:%d, father:%d, owner:%d, state: %d heap_size: %d\n", 
				cmd,
				procs[i].pid,
				procs[i].father_pid,
				procs[i].owner,
				procs[i].state,
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
