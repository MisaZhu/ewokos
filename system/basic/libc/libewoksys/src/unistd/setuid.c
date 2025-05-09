#include <unistd.h>
#include <ewoksys/syscall.h>

int setuid(uid_t uid) {
	return syscall1(SYS_PROC_SET_UID, (ewokos_addr_t)uid);
}

