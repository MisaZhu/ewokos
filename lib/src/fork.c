#include <lib/fork.h>
#include <lib/syscall.h>

int fork() {
	return syscall0(SYSCALL_FORK);
}

void exit(int code) {
	syscall1(SYSCALL_EXIT, code);
}

void wait(int pid) {
	syscall1(SYSCALL_WAIT, pid);
}

void yield() {
	syscall0(SYSCALL_YIELD);
}
