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

static const char* get_state(procinfo_t* proc) {
	static char ret[16];
	if(proc->state == 4) {
		if(proc->pid == proc->block_by)
			strcpy(ret, "blk [ipc]");
		else if(proc->block_by < 0)
			strcpy(ret, "blk [kev]");
		else 
			snprintf(ret, 15, "blk [%d]", proc->block_by);
	}
	else if(proc->state == 3)
		snprintf(ret, 15, "wat [%d]", proc->wait_for);
	else 
		strcpy(ret, _states[proc->state]);
	return ret;
}

static const char* get_owner(procinfo_t* proc) {
	if(proc->owner < 0)
		return "kernel";
	if(proc->owner == 0)
		return "root";
	static char ret[32];
	snprintf(ret, 31, "%d", proc->owner);
	return ret;
}

static const char* get_cmd(procinfo_t* proc, int full) {
	if(!full) {
		char* p = proc->cmd;
		while(*p != 0) {
			if(*p == ' ') {
				*p = 0;
				break;
			}
			p++;
		}
	}	
	return proc->cmd;
}

int main(int argc, char* argv[]) {
	int full = 0;
	int all = 0;
	if(argc == 2 && argv[1][0] == '-') {	
		if(strchr(argv[1], 'f') != NULL)
			full = 1;
		if(strchr(argv[1], 'a') != NULL)
			all = 1;
	}

	int num = 0;
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYSINFO, (int32_t)&sysinfo);
	uint32_t fr_mem = sysinfo.mem.free / (1024*1024);
	uint32_t shm_mem = sysinfo.mem.shared / (1024*1024);
	uint32_t t_mem = sysinfo.phy_mem_size / (1024*1024);
	uint32_t csec = sysinfo.kernel_sec;

	procinfo_t* procs = (procinfo_t*)syscall1(SYS_GET_PROCS, (int)&num);
	if(procs != NULL) {
		printf("OWNER    PID    FATHER  STATE       IPC_BUSY TIME       PROC\n"); 
		for(int i=0; i<num; i++) {
			if(procs[i].type != PROC_TYPE_PROC && all == 0)
				continue;

			uint32_t sec = csec - procs[i].start_sec;
			printf("%8s %4d   %6d  %10s  %8s %02d:%02d:%02d   %s\n", 
				get_owner(&procs[i]),
				procs[i].pid,
				procs[i].father_pid,
				get_state(&procs[i]),
				procs[i].ipc_busy ? "true":"false",
				sec / (3600),
				sec / 60,
				sec % 60,
				get_cmd(&procs[i], full));
		}
		free(procs);
	}
	printf("\nmemory: total %d MB, free %d MB, shm %d MB\n", t_mem, fr_mem, shm_mem);
	printf("processes: %d\n", num);
	return 0;
}
