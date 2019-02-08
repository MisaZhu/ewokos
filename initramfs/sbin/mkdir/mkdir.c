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
		char cwd[FNAME_MAX];
		int fd = fsOpen(getcwd(cwd, FNAME_MAX));
		if(fd >= 0) {
			fsAdd(fd, arg);
			fsClose(fd);
		}
	}

	exit(0);
}
