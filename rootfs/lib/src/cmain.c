#include <cmain.h>
#include <syscall.h>
#include <stdio.h>
#include <unistd.h>

static char _cmd[CMD_MAX] = { 0 };
static int _offCmd = 0;

static void init_cmain_arg() {
	_offCmd = 0;
	syscall2(SYSCALL_GET_CMD, (int)_cmd, CMD_MAX-1);
}

static char* read_cmain_arg() {
	char* p = NULL;
	bool quotes = false;

	while(_cmd[_offCmd] != 0) {
		char c = _cmd[_offCmd];
		_offCmd++;
		if(quotes) {
			if(c == '"') {
				_cmd[_offCmd-1] = 0;
				return p;
			}
			continue;
		}

		if(c == ' ') {
			if(p == NULL) {
				continue;
			}
			else {
				_cmd[_offCmd-1] = 0;
				return p;
			}
		}
		else if(p == NULL) {
			if(c == '"') {
				quotes = true;
				_offCmd++;
			}
			p = _cmd + _offCmd - 1;
		}
	}
	return p;
}

#define ARG_MAX 16

void _start() {
	char* argv[ARG_MAX];
	int32_t argc = 0;
	init_cmain_arg();

	while(argc < ARG_MAX) {
		char* arg = read_cmain_arg(); 
		if(arg == NULL || arg[0] == 0)
			break;
		argv[argc++] = arg;
	}

	init_stdio();
	int ret = main(argc, argv);
	exit(ret);
}
