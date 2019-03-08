#include <cmain.h>
#include <syscall.h>
#include <stdio.h>

static char _cmd[256] = { 0 };
static int _offCmd = 0;

void initCMainArg() {
	_offCmd = 0;
	syscall2(SYSCALL_GET_CMD, (int)_cmd, 256);
}

const char* readCMainArg() {
	const char* p = NULL;
	while(_cmd[_offCmd] != 0) {
		char c = _cmd[_offCmd];
		_offCmd++;
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
			p = _cmd + _offCmd - 1;
		}
	}
	return p;
}
