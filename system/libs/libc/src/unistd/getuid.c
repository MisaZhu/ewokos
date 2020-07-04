#include <unistd.h>
#include <sys/syscall.h>

int getuid(void) {
	return syscall0(SYS_PROC_GET_UID);
}

