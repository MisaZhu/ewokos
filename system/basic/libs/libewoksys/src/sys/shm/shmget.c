#include <sys/shm.h>
#include <ewoksys/syscall.h>

int shmget(key_t key, int size, int flag) {
	return syscall3(SYS_PROC_SHM_GET, key, size, flag);
}


