#include <unistd.h>
#include <ewoksys/syscall.h>

int setgid(int gid) {
	return syscall1(SYS_PROC_SET_GID, gid);
}

