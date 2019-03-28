#include <unistd.h>
#include <stdio.h>
#include <syscall.h>

int main() {
	int uid = syscall0(SYSCALL_GET_UID);
	printf("%d\n", uid);
	return 0;
}
