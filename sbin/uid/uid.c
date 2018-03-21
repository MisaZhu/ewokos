#include <unistd.h>
#include <stdio.h>
#include <syscall.h>

void _start()
{
	int uid = syscall0(SYSCALL_GET_UID);
	printf("%d\n", uid);
	exit(0);
}
