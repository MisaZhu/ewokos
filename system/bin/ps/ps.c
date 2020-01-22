#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sysinfo.h>
#include <string.h>

static const char* _states[] = {
	"unu",
	"crt",
	"slp",
	"wat",
	"blk",
	"rdy",
	"run",
	"zmb"
};

static const char* get_cmd(char* cmd, int full) {
	if(full)
		return cmd;

	char* p = cmd;
	while(*p != 0) {
		if(*p == ' ') {
			*p = 0;
			break;
		}
		p++;
	}
	return cmd;
}

int main(int argc, char* argv[]) {
	int full = 0;
	if(argc == 2 && argv[1][0] == '-') {	
		if(strchr(argv[1], 'f') != NULL)
			full = 1;
	}

	int num = 0;
	sysinfo_t sysinfo;
	syscall1(SYS_GET_SYSINFO, (int32_t)&sysinfo);
	uint32_t fr_mem = sysinfo.free_mem / (1024*1024);
	uint32_t shm_mem = sysinfo.shm_mem / (1024*1024);
	uint32_t t_mem = sysinfo.total_mem / (1024*1024);
	uint32_t csec = sysinfo.kernel_tic;

	procinfo_t* procs = (procinfo_t*)syscall1(SYS_GET_PROCS, (int)&num);
	if(procs != NULL) {
		printf("  PID    FATHER OWNER   STATE TIME       HEAP(k)  PROC\n"); 
		for(int i=0; i<num; i++) {
			if(procs[i].type == PROC_TYPE_THREAD && full == 0)
				continue;

			uint32_t sec = csec - procs[i].start_sec;
			printf("  %4d   %6d %5d   %5s %02d:%02d:%02d   %8d %s%s\n", 
				procs[i].pid,
				procs[i].father_pid,
				procs[i].owner,
				_states[procs[i].state],
				sec / (3600),
				sec / 60,
				sec % 60,
				procs[i].heap_size/1024,
				get_cmd(procs[i].cmd, full),
				procs[i].type == PROC_TYPE_THREAD ? " [t]":"");
		}
		free(procs);
	}
	printf("  memory: total %d MB, free %d MB, shm %d MB\n", t_mem, fr_mem, shm_mem);
	printf("  processes: %d\n", num);
	return 0;
}
