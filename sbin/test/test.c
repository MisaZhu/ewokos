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

	int uid = usermanAuth("user", "passwd");
	printf("uid: %d\n", uid);
	uid  = syscall0(SYSCALL_GET_UID);
	printf("uid: %d\n", uid);

	exit(0);
}
