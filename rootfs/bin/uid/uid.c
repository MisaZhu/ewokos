#include <unistd.h>
#include <stdio.h>
#include <syscall.h>

int main() {
	int uid = syscall1(SYSCALL_GET_UID, getpid());
	printf("%d\n", uid);
	return 0;
}
