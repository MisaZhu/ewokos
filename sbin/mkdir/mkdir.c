#include <unistd.h>
#include <stdio.h>
#include <cmain.h>
#include <kserv/fs.h>

void _start()
{
	initCMainArg();
	const char* cmd = readCMainArg();
	cmd = readCMainArg();
	printf("mkdir: [%s]\n", cmd);

	if(cmd != NULL) {
		char cwd[FNAME_MAX];
		int fd = fsOpen(getcwd(cwd, FNAME_MAX));
		if(fd >= 0) {
			fsAdd(fd, cmd);
			fsClose(fd);
		}
	}

	exit(0);
}
