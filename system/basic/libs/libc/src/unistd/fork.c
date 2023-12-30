#include <unistd.h>
#include <ewoksys/syscall.h>

int fork(void) {
	return syscall0(SYS_FORK);
}

