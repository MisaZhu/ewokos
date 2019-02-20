#include <unistd.h>
#include <stdio.h>
#include <cmain.h>
#include <kserv/fs.h>

void _start()
{
	initCMainArg();
	const char* arg = readCMainArg();
	arg = readCMainArg();

	if(arg != NULL) {
		char cwd[NAME_MAX];
		int fd = fsOpen(getcwd(cwd, NAME_MAX), 0);
		if(fd >= 0) {
			fsAdd(fd, arg);
			fsClose(fd);
		}
	}

	exit(0);
}
