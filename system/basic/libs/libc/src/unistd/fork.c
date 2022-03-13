#include <unistd.h>
#include <sys/syscall.h>

int fork(void) {
	return syscall0(SYS_FORK);
}

