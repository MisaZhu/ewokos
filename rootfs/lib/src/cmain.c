#include <cmain.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <device.h>

static char _cmd[1024] = { 0 };
static int _off_cmd = 0;

static void init_cmain_arg() {
	_off_cmd = 0;
	syscall3(SYSCALL_GET_CMD, getpid(), (int)_cmd, 1023);
}

static char* read_cmain_arg() {
	char* p = NULL;
	bool quotes = false;

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
				quotes = true;
				_off_cmd++;
			}
			p = _cmd + _off_cmd - 1;
		}
	}
	return p;
}

#define ARG_MAX 16

void _start() {
	init_stdout_buffer();

	char* argv[ARG_MAX];
	int32_t argc = 0;
	init_cmain_arg();
	while(argc < ARG_MAX) {
		char* arg = read_cmain_arg(); 
		if(arg == NULL || arg[0] == 0)
			break;
		argv[argc++] = arg;
	}

	int ret = main(argc, argv);
	close(0);
	close(1);
	exit(ret);
}
