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

/* How long to wait for a terminal to answer the cursor-position report.
 * A real terminal replies within a few milliseconds even over a slow serial
 * line, so the timeout only bounds the wait for terminals that never answer. */
#define TOP_SIZE_PROBE_TIMEOUT_MS 200

// Read the "ESC [ <row> ; <col> R" cursor-position report from stdin.
// Returns true and fills *rows/*cols on success, false on timeout/garbage.
static bool read_cursor_pos_reply(uint32_t* rows, uint32_t* cols) {
	enum { WANT_ESC, WANT_BRACKET, WANT_PARAMS } state = WANT_ESC;
	uint32_t row = 0, col = 0;
	bool parsing_col = false;

	for(;;) {
		struct pollfd pfd;
		pfd.fd = 0; //stdin
		pfd.events = POLLIN;
		pfd.revents = 0;
		// The opening ESC may never arrive; the bytes after it come together.
		int wait_ms = (state == WANT_ESC) ? TOP_SIZE_PROBE_TIMEOUT_MS : 100;
		if(poll(&pfd, 1, wait_ms) <= 0)
			return false;

		char c;
		if(read(0, &c, 1) != 1)
			return false;

		switch(state) {
		case WANT_ESC:
			if(c == '\033')
				state = WANT_BRACKET;
			break;
		case WANT_BRACKET:
			if(c == '[')
				state = WANT_PARAMS;
			else if(c != '\033')
				return false;
			break;
		case WANT_PARAMS:
			if(c >= '0' && c <= '9') {
				if(parsing_col)
					col = col*10 + (c - '0');
				else
					row = row*10 + (c - '0');
			}
			else if(c == ';')
				parsing_col = true;
			else if(c == 'R') {
				*rows = row;
				*cols = col;
				return true;
			}
			else
				return false;
			break;
		}
	}
}

static uint32_t _term_rows = 0;
static uint32_t _term_cols = 0;

static void get_screen_size(uint32_t* rows, uint32_t* cols) {
	// Preferred path: ask the terminal driver for its window size directly.
	// The GUI consoles (consoled/xterm) publish their live textgrid geometry
	// through TIOCGWINSZ. This is a single synchronous IPC with no round-trip
	// race, so re-query every refresh to track window resizes.
	if(isatty(0)) {
		struct winsize ws;
		memset(&ws, 0, sizeof(ws));
		if(ioctl(0, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
			_term_rows = ws.ws_row;
			_term_cols = ws.ws_col;
		}
	}

	if(_term_rows == 0 || _term_cols == 0) {
		// Fallback: ask the terminal itself over the wire. Serial consoles
		// (/dev/tty0) have no window size to report, so park the cursor at an
		// absurd row/col (it clamps to the real bottom-right corner) and request
		// a cursor-position report. This is how the geometry of whatever emulator
		// is attached (e.g. macOS Terminal) is revealed. Probed only once: it
		// consumes stdin, so running it per refresh could swallow key input.
		if(isatty(0) && isatty(1)) { //stdin and stdout are the terminal
			uint32_t r = 0, c = 0;
			printf(ESC "[999;999H" ESC "[6n");
			if(read_cursor_pos_reply(&r, &c) && r > 0 && c > 0) {
				_term_rows = r;
				_term_cols = c;
			}
			// we moved the cursor to the corner; put it back home
			printf(ESC_CURSOR_TOPLEFT);
		}
		if(_term_rows == 0)
			_term_rows = 24; //VT100 fallback for a silent terminal
		if(_term_cols == 0)
			_term_cols = 80;
	}

	*rows = _term_rows;
	*cols = _term_cols;
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

	uint32_t rows, cols;
	get_screen_size(&rows, &cols);

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

	int list_max = (rows > 4) ? (int)rows - 4 : 1; //2 summary + 1 title + 1 spare
	if(shown > list_max)
		shown = list_max;

	int tail_max = (int)cols - 60; //the fixed columns before PROC take 60 chars
	if(tail_max < 0)
		tail_max = 0;

	for(int i=0; i<shown; i++) {
		procinfo_t* proc = entries[i].proc;
		uint32_t sec = csec - proc->start_sec;
		char heap_size[32] = {0};
		char shm_size[32] = {0};
		char tail[PROC_INFO_MAX_CMD_LEN + 32];
		snprintf(tail, sizeof(tail), "%s", get_cmd(proc));

		if(proc->type == TASK_TYPE_THREAD) {
			int n = strlen(tail);
			snprintf(tail + n, sizeof(tail) - n, " [THRD:%d]", proc->father_pid);
		}
		else {
			int tnum = (thread == 0) ? get_thread_num(procs, num, proc) : 0;
			if(tnum > 0) {
				int n = strlen(tail);
				snprintf(tail + n, sizeof(tail) - n, " [%dt]", tnum);
			}
		}

		//truncate the PROC column so a long path can never wrap the line and
		//corrupt the full-screen redraw
		printf("%-8s %-4d %-4d  %-6s %-9s %02d:%02d:%02d %-6s  %-5s  %.*s" ESC_CLEAR2EOL "\n",
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
			tail_max, tail);
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
