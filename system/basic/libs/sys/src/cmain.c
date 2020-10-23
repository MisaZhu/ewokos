#include <sys/cmain.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


static char _cmd[1024];
static int _off_cmd;

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

static void close_stdio(void) {
	close(0);
	close(1);
}

static void init_cmd(void) {
	_cmd[0] = 0;
	_off_cmd = 0;
	syscall3(SYS_PROC_GET_CMD, getpid(), (int32_t)_cmd, 1023);
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

void _start(void) {
	char* argv[ARG_MAX];
	int32_t argc = 0;

	sys_signal_init();
	proc_init();
	init_stdio();
	init_cmd();

	while(argc < ARG_MAX) {
		char* arg = read_cmain_arg(); 
		if(arg == NULL || arg[0] == 0)
			break;
		argv[argc++] = arg;
	}

	int ret = main(argc, argv);
	close_stdio();
	exit(ret);
}

#ifdef __cplusplus
}
#endif

