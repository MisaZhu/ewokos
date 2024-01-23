#include <unistd.h>
#include <ewoksys/syscall.h>

int getgid(void) {
	return syscall0(SYS_PROC_GET_GID);
}

