#include <unistd.h>
#include <sys/syscall.h>

int vfork(void) {
	return syscall0(SYS_FORK);
}

