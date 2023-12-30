#include <unistd.h>
#include <ewoksys/syscall.h>

int vfork(void) {
	return syscall0(SYS_FORK);
}

