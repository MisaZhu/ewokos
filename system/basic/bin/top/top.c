#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/sys.h>
#include <sysinfo.h>
#include <string.h>
#include <ewoksys/session.h>

#define ESC "\033"
#define ESC_CURSOR_HIDE     ESC "[?25l"
#define ESC_CURSOR_SHOW     ESC "[?25h"
#define ESC_CURSOR_TOPLEFT  ESC "[H"
#define ESC_CLEAR_SCREEN    ESC "[2J"
#define ESC_CLEAR2EOL       ESC "[K"
#define ESC_CLEAR2EOS       ESC "[J"

static const char* _states[] = {
	"unu", //0
	"crt", //1
	"slp", //2
	"wat", //3
	"blk", //4
	"rdy", //5
	"run", //6
	"zmb"  //7
};

typedef struct {
	procinfo_t* proc;
	uint32_t    load; //run_usec, threads aggregated into the process
} task_entry_t;

static const char* get_state(procinfo_t* proc) {
	static char ret[16];
	if(proc->state == 4) {
		strcpy(ret, "blk");
	}
	else if(proc->state == 3)
		snprintf(ret, 13, "wat[%d]", proc->wait_for);
	else {
		strcpy(ret, _states[proc->state]);
		if((proc->state == 5 || proc->state == 6) && proc->priority > 0) {
			char tmp[32];
			snprintf(tmp, sizeof(tmp), "%s(%d)", ret, proc->priority);
			strcpy(ret, tmp);
		}
	}
	return ret;
}

static const char* get_owner(procinfo_t* proc) {
	if(proc->uid < 0)
		return "kernel";

	session_info_t info;
	static char name[SESSION_USER_MAX+1] = {0};

	if(session_get_by_uid(proc->uid, &info) == 0)
		strncpy(name, info.user, SESSION_USER_MAX);
	else
		snprintf(name, SESSION_USER_MAX, "%d", proc->uid);

	return name;
}

static const char* get_cmd(procinfo_t* proc) {
	char* p = proc->cmd;
	while(*p != 0) { //trim arguments, keep the command name only
		if(*p == ' ') {
			*p = 0;
			break;
		}
		p++;
	}
	return proc->cmd;
}

static const char* get_core_loading(sys_info_t* sys_info, procinfo_t* proc, uint32_t run_usec) {
	static char ret[16];
	if(sys_info->cores > 1)
		snprintf(ret, sizeof(ret), "%d:%d%%", proc->core, run_usec/10000);
	else
		snprintf(ret, sizeof(ret), "%d%%", run_usec/10000);
	return ret;
}

static uint32_t get_loading(procinfo_t* procs, int num, procinfo_t* proc, int8_t thread) {
	uint32_t run_usec = proc->run_usec;
	if(thread == 0 && proc->type == TASK_TYPE_PROC) { //aggregate all thread loadings
		for(int i=0; i<num; i++) {
			procinfo_t* p = &procs[i];
			if(p->type == TASK_TYPE_THREAD && p->father_pid == proc->pid)
				run_usec += p->run_usec;
		}
	}
	return run_usec;
}

static int get_thread_num(procinfo_t* procs, int num, procinfo_t* proc) {
	int n = 0;
	for(int i=0; i<num; i++) {
		procinfo_t* p = &procs[i];
		if(p->type == TASK_TYPE_THREAD && p->father_pid == proc->pid)
			n++;
	}
	return n;
}

static int comp_load(const void* a, const void* b) {
	const task_entry_t* ea = (const task_entry_t*)a;
	const task_entry_t* eb = (const task_entry_t*)b;
	if(eb->load != ea->load)
		return (eb->load > ea->load) ? 1 : -1; //cpu loading, descendant
	return ea->proc->pid - eb->proc->pid; //stable order for equals
}

static uint32_t get_screen_rows(void) {
	if(isatty(0)) {
		struct winsize ws;
		memset(&ws, 0, sizeof(ws));
		if(ioctl(0, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0)
			return ws.ws_row;
	}
	return 24; //VT100 fallback for a silent terminal
}

static void refresh(int8_t thread) {
	sys_info_t sys_info;
	sys_state_t sys_state;
	sys_get_sys_info(&sys_info);
	syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)&sys_state);
	uint32_t fr_mem = sys_state.mem.free / (1024*1024);
	uint32_t shm_mem = sys_state.mem.shared / (1024*1024);
	uint32_t t_mem = sys_info.total_usable_mem_size / (1024*1024);
	uint32_t csec = (uint32_t)(sys_state.kernel_usec / 1000000);

	int num = syscall0(SYS_GET_PROCS_NUM);
	procinfo_t* procs = NULL;
	task_entry_t* entries = NULL;
	if(num > 0) {
		procs = (procinfo_t*)malloc(sizeof(procinfo_t)*num);
		entries = (task_entry_t*)malloc(sizeof(task_entry_t)*num);
	}
	if(procs == NULL || entries == NULL) {
		if(procs != NULL) free(procs);
		if(entries != NULL) free(entries);
		return;
	}

	int got = syscall2(SYS_GET_PROCS, (ewokos_addr_t)num, (ewokos_addr_t)procs);
	if(got < 0) {
		free(procs);
		free(entries);
		return;
	}
	num = got;

	int shown = 0;
	for(int i=0; i<num; i++) {
		procinfo_t* proc = &procs[i];
		if(proc->type != TASK_TYPE_PROC && thread == 0) //for thread
			continue;
		entries[shown].proc = proc;
		entries[shown].load = get_loading(procs, num, proc, thread);
		shown++;
	}
	qsort(entries, shown, sizeof(task_entry_t), comp_load);

	printf(ESC_CURSOR_TOPLEFT);
	printf("\033[1mtasks: %d, up %02d:%02d:%02d, memory: total %d MB, free %d MB, shm %d MB" ESC_CLEAR2EOL "\n",
			num, csec/3600, (csec%3600)/60, csec%60, t_mem, fr_mem, shm_mem);
	printf("cpu idle:");
	for(uint32_t i=0; i<sys_info.cores; i++) {
		int idle = sys_info.core_idles[i]/10000;
		printf("  %d%%", idle > 100 ? 100 : idle);
	}
	printf("      ('q' quit, 't' threads)\033[0m" ESC_CLEAR2EOL "\n");
	printf("\033[7mOWNER    PID  FATH  CORE   STATE     TIME     HEAP    SHM    PROC" ESC_CLEAR2EOL "\033[0m\n");

	uint32_t rows = get_screen_rows();
	int list_max = (rows > 4) ? (int)rows - 4 : 1; //2 summary + 1 title + 1 spare
	if(shown > list_max)
		shown = list_max;

	for(int i=0; i<shown; i++) {
		procinfo_t* proc = entries[i].proc;
		uint32_t sec = csec - proc->start_sec;
		char heap_size[32] = {0};
		char shm_size[32] = {0};
		printf("%-8s %-4d %-4d  %-6s %-9s %02d:%02d:%02d %-6s  %-5s  %s",
			get_owner(proc),
			proc->pid,
			proc->father_pid,
			get_core_loading(&sys_info, proc, entries[i].load),
			get_state(proc),
			sec / 3600,
			(sec % 3600) / 60,
			sec % 60,
			get_mem_size_desc(proc->heap_size, heap_size),
			get_mem_size_desc(proc->shm_size, shm_size),
			get_cmd(proc));

		if(proc->type == TASK_TYPE_THREAD)
			printf(" [THRD:%d]", proc->father_pid);
		else {
			int tnum = (thread == 0) ? get_thread_num(procs, num, proc) : 0;
			if(tnum > 0)
				printf(" [%dt]", tnum);
		}
		printf(ESC_CLEAR2EOL "\n");
	}
	printf(ESC_CLEAR2EOS);

	free(procs);
	free(entries);
}

static int doargs(int argc, char* argv[], int8_t* thread, int* delay_ms) {
	*thread = 0;
	*delay_ms = 2000;

	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "td:");
		if(c == -1)
			break;

		switch (c) {
			case 't':
				*thread = 1;
				break;
			case 'd': {
				int sec = atoi(optarg);
				if(sec > 0)
					*delay_ms = sec * 1000;
				break;
			}
			case '?':
				fprintf(stderr, "Usage: top [-t] [-d seconds]\n");
				return -1;
			default:
				c = -1;
				break;
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	int8_t thread;
	int delay_ms;
	if(doargs(argc, argv, &thread, &delay_ms) != 0)
		return -1;

	printf(ESC_CURSOR_HIDE ESC_CURSOR_TOPLEFT ESC_CLEAR_SCREEN);

	while(1) {
		refresh(thread);

		//wait for the next refresh, waked up early by key input
		struct pollfd pfd;
		pfd.fd = 0; //stdin
		pfd.events = POLLIN;
		pfd.revents = 0;
		if(poll(&pfd, 1, delay_ms) > 0) {
			char c = 0;
			if(read(0, &c, 1) == 1) {
				if(c == 'q' || c == 'Q' || c == 0x03) //'q' or ctrl-c
					break;
				if(c == 't' || c == 'T')
					thread = !thread;
			}
		}
	}

	printf(ESC_CURSOR_SHOW ESC_CURSOR_TOPLEFT ESC_CLEAR_SCREEN);
	return 0;
}
