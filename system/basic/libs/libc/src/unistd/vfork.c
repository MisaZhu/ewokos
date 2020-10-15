#include <unistd.h>
#include <sys/syscall.h>

int vfork(void) {
	return syscall1(SYS_FORK, 1);
}

