#include <pmessage.h>
#include <fork.h>
#include <malloc.h>
#include <stdio.h>
#include <kserv/fs.h>
#include <kserv/kserv.h>
#include <kserv/fs.h>
#include <string.h>

void _start()
{
	int fd = fsOpen("/dev/tty0");
	if(fd >= 0) {
		fsWrite(fd, "hello\n", 6);
		fsClose(fd);
	}

	exit(0);
}
