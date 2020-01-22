#include <sys/syscall.h>

int waitpid(int pid) {
	return syscall1(SYS_WAIT_PID, pid);
}
