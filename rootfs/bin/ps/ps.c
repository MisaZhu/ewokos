#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <kstring.h>
#include <cmain.h>

static const char* _states[] = {
	"unu",
	"crt",
	"slp",
	"wat",
	"blk",
	"rdy",
	"run",
	"tmn"
};

static const char* get_cmd(int32_t pid, bool full) {
	static char cmd[1024];
	syscall3(SYSCALL_GET_CMD, pid, (int)cmd, 1023);
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
	bool full = false;
	if(argc == 2 && argv[1][0] == '-') {	
		if(strchr(argv[1], 'f') != NULL)
			full = true;
	}

	int num = 0;
	uint32_t fr_mem = (uint32_t)syscall2(SYSCALL_SYSTEM_CMD, 1, 0) / KB;
	uint32_t t_mem = (uint32_t)syscall2(SYSCALL_SYSTEM_CMD, 0, 0) / MB;
	uint32_t csec;
	syscall3(SYSCALL_SYSTEM_CMD, 4, (int32_t)&csec, 0);

	proc_info_t* procs = (proc_info_t*)syscall2(SYSCALL_SYSTEM_CMD, 2, (int)&num);
	if(procs != NULL) {
		printf("  PID    FATHER OWNER   STATE TIME       HEAP(k)  PROC\n"); 
		for(int i=0; i<num; i++) {
			uint32_t sec = csec - procs[i].start_sec;
			printf("  %4d   %6d %5d   %5s %02d:%02d:%02d   %8d %s\n", 
				procs[i].pid,
				procs[i].father_pid,
				procs[i].owner,
				_states[procs[i].state],
				sec / (3600),
				sec / 60,
				sec % 60,
				procs[i].heap_size/1024,
				get_cmd(procs[i].pid, full));
		}
		free(procs);
	}
	if(fr_mem > 1024)
		printf("  memory: total %d MB, free %d KB (%d MB)\n", t_mem, fr_mem, fr_mem/1024);
	else
		printf("  memory: total %d MB, free %d KB\n", t_mem, fr_mem);
	printf("  processes: %d\n", num);
	return 0;
}
