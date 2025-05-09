#include <unistd.h>
#include <ewoksys/syscall.h>

int setgid(gid_t gid) {
	return syscall1(SYS_PROC_SET_GID, (ewokos_addr_t)gid);
}

