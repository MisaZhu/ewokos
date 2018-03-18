#include <unistd.h>
#include <stdio.h>
#include <kserv/fs.h>

void _start()
{
	char* p = (char*)0xFFFFFFFF;
	printf("%s\n", p);

	int fd = fsOpen("/dev/tty0");
	if(fd >= 0) {
		fsWrite(fd, "hello\n", 6);
		fsClose(fd);
	}

	exit(0);
}
