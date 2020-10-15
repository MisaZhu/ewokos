#include <unistd.h>
#include <sys/syscall.h>

int fork(void) {
	return syscall1(SYS_FORK, 0);
}

