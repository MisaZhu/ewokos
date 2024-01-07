#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>
#include <string.h>
#include <ewoksys/session.h>

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
			strcpy(ret, "blk[ipc]");
		else if(proc->block_by < 0)
			strcpy(ret, "blk[kev]");
		else 
			snprintf(ret, 13, "blk[%d]", proc->block_by);
	}
	else if(proc->state == 3)
		snprintf(ret, 13, "wat[%d]", proc->wait_for);
	else 
		strcpy(ret, _states[proc->state]);
	return ret;
}

static const char* get_owner(procinfo_t* proc) {
	if(proc->uid < 0)
		return "kernel";

	session_info_t info;
	static char name[SESSION_USER_MAX+1];

	if(session_get(proc->uid, &info) == 0)
		sstrncpy(name, info.user, SESSION_USER_MAX);
	else
		snprintf(name, SESSION_USER_MAX, "%d", proc->uid);

	return name;
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

static const char* get_core_loading(sys_info_t* sys_info, procinfo_t* proc) {
	static char ret[8];
	if(sys_info->cores > 1)
		snprintf(ret, 7, "%d:%d%%", proc->core, proc->run_usec/10000);
	else
		snprintf(ret, 7, "%d%%", proc->run_usec/10000);
	return ret;
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
	uint32_t core_idle[16]; //max 16 cores;
	sys_info_t sys_info;
	sys_state_t sys_state;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sys_info);
	syscall1(SYS_GET_SYS_STATE, (int32_t)&sys_state);
	uint32_t fr_mem = sys_state.mem.free / (1024*1024);
	uint32_t shm_mem = sys_state.mem.shared / (1024*1024);
	uint32_t t_mem = sys_info.phy_mem_size / (1024*1024);
	uint32_t csec = sys_state.kernel_sec;

	procinfo_t* procs = (procinfo_t*)syscall1(SYS_GET_PROCS, (int)&num);
	if(procs != NULL) {
		if(full)
			printf("OWNER   PID  FATH CORE   STATE     TIME     HEAP(K) SHM(K) PROC\n"); 
		else
			printf("OWNER   PID  FATH CORE   STATE     PROC\n"); 
		for(int i=0; i<num; i++) {
			procinfo_t* proc = &procs[i];
			if(strcmp(proc->cmd, "cpu_core_halt") == 0) {
				core_idle[proc->core] = proc->run_usec;
				continue;
			}

			if(proc->type != PROC_TYPE_PROC && all == 0) //for thread 
				continue;

			if(full) {
				uint32_t sec = csec - proc->start_sec;
				printf("%8s%4d %4d %6s %9s %02d:%02d:%02d %6d  %5d  %s",
					get_owner(proc),
					proc->pid,
					proc->father_pid,
					get_core_loading(&sys_info, proc),
					get_state(proc),
					sec / (3600),
					sec / 60,
					sec % 60,
					proc->heap_size / 1024,
					proc->shm_size / 1024,
					get_cmd(proc, full));
			}
			else {
				printf("%8s%4d %4d %6s %9s %s",
					get_owner(proc),
					proc->pid,
					proc->father_pid,
					get_core_loading(&sys_info, proc),
					get_state(proc),
					get_cmd(proc, full));

			}

			if(proc->type == PROC_TYPE_THREAD)
				printf(" [THRD:%d]\n", proc->father_pid);
			else
				printf("\n");
		}
		free(procs);
	}
	printf("\nmemory: total %d MB, free %d MB, shm %d MB\n", t_mem, fr_mem, shm_mem);

	printf("cpu idle:");
	for(int i=0; i<sys_info.cores; i++) {
		int idle = core_idle[i]/10000;
		printf("  %d%%", idle > 100 ? 100 : idle);
	}
	printf("\n");
	return 0;
}
