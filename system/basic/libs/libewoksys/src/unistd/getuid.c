#include <unistd.h>
#include <ewoksys/syscall.h>

uid_t getuid(void) {
	return syscall0(SYS_PROC_GET_UID);
}

