#include <unistd.h>
#include <ewoksys/syscall.h>

gid_t getgid(void) {
	return syscall0(SYS_PROC_GET_GID);
}

