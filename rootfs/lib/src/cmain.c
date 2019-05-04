#include <cmain.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <device.h>

static char _cmd[CMD_MAX] = { 0 };
static int _off_cmd = 0;

static void init_cmain_arg() {
	_off_cmd = 0;
	syscall2(SYSCALL_GET_CMD, (int)_cmd, CMD_MAX-1);
}

static void do_redir(const char* fname, bool in) {
	if(in) {
		int32_t fd = open(fname, O_RDONLY);
		if(fd < 0) {
			printf("error: '%s' open failed!\n", fname);
			exit(-1);
		}
		dup2(fd, 0);
	}
	else {
		int32_t fd = open(fname, O_WRONLY | O_CREAT);
		if(fd < 0) {
			printf("error: '%s' open failed!\n", fname);
			exit(-1);
		}
		dup2(fd, 1);
	}
}

static char* read_cmain_arg() {
	char* p = NULL;
	bool quotes = false;
	bool redir = false;

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
			else if(c == '>') { //if start of redirection
				redir = true;
				_off_cmd++;
				p = NULL;
				continue;
			}
			p = _cmd + _off_cmd - 1;
		}
	}

	if(redir && p != NULL) {
		do_redir(p, false);	 //redir stdout
		p = NULL;
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
	exit(ret);
}
