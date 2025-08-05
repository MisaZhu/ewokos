#include <ewoksys/cmain.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ewoksys/syscall.h>
#include <ewoksys/signal.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/ipc.h>
#include <ewoksys/sys.h>
#include <ewoksys/core.h>
#include <procinfo.h>
#include <unistd.h>
#include <setenv.h>

#ifdef __cplusplus
extern "C" {
#endif

vsyscall_info_t* _vsyscall_info = NULL;
int _current_pid = -1;

static char _cmd[PROC_INFO_MAX_CMD_LEN];
static int _off_cmd;
static const char* _argv0;

static char* read_cmain_arg(void) {
	char* p = NULL;
	uint8_t quotes = 0;

	while(_cmd[_off_cmd] != 0) {
		char c = _cmd[_off_cmd];
		_off_cmd++;
		if(quotes) { //read whole quotes content.
			if(c == '"') {
				_cmd[_off_cmd-1] = 0;
				return p;
			}
			continue;
		}
		if(c == ' ') { //read next arg.
			if(p == NULL) //skip begin spaces.
				continue;
			_cmd[_off_cmd-1] = 0;
			break;
		}
		else if(p == NULL) {
			if(c == '"') { //if start of quotes.
				quotes = 1;
				_off_cmd++;
			}
			p = _cmd + _off_cmd - 1;
		}
	}
	return p;
}

const char* cmain_get_work_dir(void) {
	static char ret[PROC_INFO_MAX_CMD_LEN] = {0};
	int i = strlen(_argv0) - 1;
	while(i >= 0 && i < PROC_INFO_MAX_CMD_LEN) {
		if(_argv0[i] == '/') {
			strncpy(ret, _argv0, i);
			ret[i] = 0;
			return ret;
		}
		i--;
	}
	return "";
}

static void close_stdio(void) {
	close(0);
	close(1);
	close(2);
}

static void init_cmd(void) {
	_cmd[0] = 0;
	_off_cmd = 0;
	_argv0 = "";
	syscall3(SYS_PROC_GET_CMD, (ewokos_addr_t)getpid(), (ewokos_addr_t)_cmd, PROC_INFO_MAX_CMD_LEN);
}

#define ARG_MAX 16
extern int __bss_start__;
extern int __bss_end__;

static void loadenv(void) {
	proto_t out;
	PF->init(&out);
	int core_pid = syscall0(SYS_CORE_PID);
	int res = ipc_call(core_pid, CORE_CMD_GET_ENVS, NULL, &out);
	if(res != 0) {
		PF->clear(&out);
		return;
	}
		
	int n = proto_read_int(&out);
	if(n > 0) {
		for(int i=0; i<n; i++) {
			const char* name = proto_read_str(&out);
			const char* value = proto_read_str(&out);
			setenv(name, value);
		}
	}
	PF->clear(&out);
}

static int set_stderr(void) {
	const char* dev = getenv("STDERR_DEV");
	if(dev == NULL || dev[0] == 0)
		return -1;

	int fd = open(dev, O_RDWR);
	if(fd > 0) {
		dup2(fd, 2);
		close(fd);
		return 0;
	}
	return -1;
}


void _libc_init(void);
void _libc_exit(void);

void _start(void) {
	char* argv[ARG_MAX] = {0};
	int32_t argc = 0;
	//clean bss befor cmain
	int *p = &__bss_start__;
	while(p < &__bss_end__){
		*p++ = 0;
	}

	_current_pid = -1;
	_current_pid = getpid();

	_vsyscall_info = (vsyscall_info_t*)syscall0(SYS_GET_VSYSCALL_INFO);
	if(_vsyscall_info == NULL)
		exit(-1);

	_libc_init();
	//__ewok_malloc_init();
	proc_init();
	sys_signal_init();
	vfs_init();
	init_cmd();

	while(argc < ARG_MAX) {
		char* arg = read_cmain_arg(); 
		if(arg == NULL || arg[0] == 0)
			break;
		if(argc == 0)
			_argv0 = arg;
		argv[argc] = (char*)malloc(strlen(arg)+1);
		if(argv[argc] != NULL)
			strcpy(argv[argc], arg);
		argc++;
	}

	// int val = setenv("PATH", "/bin", 1);
	// klog("setenv: %d\n", val);
	// // const char* paths = getenv("PATH");
	// // klog("PATH: %s\n", paths);

	loadenv();
	set_stderr();
	
	int ret = main(argc, argv);
	close_stdio();
	//__ewok_malloc_close();
	proc_exit();
	_libc_exit();

	argc = 0;
	while(argc < ARG_MAX) {
		if(argv[argc] != NULL)
			free(argv[argc]);
		argc++;
	}
	exit(ret);
}

#ifdef __cplusplus
}
#endif

