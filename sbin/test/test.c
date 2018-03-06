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
	int fd = fsOpen("/initrd/test");
	printf("fd: %d\n", fd);
	exit(0);
}
