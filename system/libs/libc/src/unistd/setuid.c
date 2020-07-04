#include <unistd.h>
#include <sys/syscall.h>

int setuid(int uid) {
	return syscall1(SYS_PROC_SET_UID, uid);
}

