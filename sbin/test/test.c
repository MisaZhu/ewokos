#include <unistd.h>
#include <stdio.h>
#include <syscall.h>
#include <kserv/fs.h>
#include <kserv/userman.h>

void _start()
{
	int fd = fsOpen("/dev/tty0");
	if(fd >= 0) {
		fsWrite(fd, "hello\n", 6);
		fsClose(fd);
	}

	exit(0);
}
