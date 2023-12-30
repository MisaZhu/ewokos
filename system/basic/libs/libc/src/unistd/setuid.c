#include <unistd.h>
#include <ewoksys/syscall.h>

int setuid(int uid) {
	return syscall1(SYS_PROC_SET_UID, uid);
}

