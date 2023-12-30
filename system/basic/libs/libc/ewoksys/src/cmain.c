#include <ewoksys/cmain.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ewoksys/syscall.h>
#include <ewoksys/signal.h>
#include <ewoksys/proc.h>
#include <procinfo.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


static char _cmd[PROC_INFO_CMD_MAX];
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
	static char ret[PROC_INFO_CMD_MAX];
	int i = strlen(_argv0) - 1;
	while(i >= 0) {
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
	syscall3(SYS_PROC_GET_CMD, getpid(), (int32_t)_cmd, PROC_INFO_CMD_MAX);
}

FILE* stdin = NULL;
FILE* stdout = NULL;
FILE* stderr = NULL;

static FILE _stdin;
static FILE _stdout;
static FILE _stderr;

static void init_stdio(void) {
	_stdin.fd = 0;
	_stdin.oflags = O_RDONLY;
	stdin = &_stdin;

	_stdout.fd = 1;
	_stdout.oflags = O_WRONLY;
	stdout = &_stdout;

	_stderr.fd = 2;
	_stderr.oflags = O_WRONLY;
	stderr = &_stderr;
}

#define ARG_MAX 16
extern int __bss_start__;
extern int __bss_end__;

void _start(void) {
	const char* argv[ARG_MAX];
	int32_t argc = 0;

	//clean bss befor cmain
	int *p = &__bss_start__;
	while(p < &__bss_end__){
		*p++ = 0;
	}

	__malloc_init();
	sys_signal_init();
	vfs_init();
	proc_init();
	init_stdio();
	init_cmd();


	while(argc < ARG_MAX) {
		char* arg = read_cmain_arg(); 
		if(arg == NULL || arg[0] == 0)
			break;
		if(argc == 0)
			_argv0 = arg;
		argv[argc++] = arg;
	}

	int ret = main(argc, argv);
	close_stdio();
	__malloc_close();
	exit(ret);
}

#ifdef __cplusplus
}
#endif

