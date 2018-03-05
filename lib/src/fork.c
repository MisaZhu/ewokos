#include <fork.h>
#include <syscall.h>

int fork() {
	return syscall0(SYSCALL_FORK);
}

int getpid() {
	return syscall0(SYSCALL_GETPID);
}

int exec(const char* cmd) {
	return syscall1(SYSCALL_EXEC, (int)cmd);
}

int execElf(const char* img) {
	return syscall1(SYSCALL_EXEC_ELF, (int)img);
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
