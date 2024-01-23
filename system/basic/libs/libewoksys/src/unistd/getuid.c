#include <unistd.h>
#include <ewoksys/syscall.h>

int getuid(void) {
	return syscall0(SYS_PROC_GET_UID);
}

