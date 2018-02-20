#include <lib/fork.h>
#include <lib/syscall.h>

int fork() {
	return syscall0(SYSCALL_FORK);
}

void exit(int code) {
	syscall1(SYSCALL_EXIT, code);
}
