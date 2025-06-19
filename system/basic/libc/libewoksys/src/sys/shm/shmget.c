#include <sys/shm.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

int shmget(key_t key, int size, int flag) {
	return syscall3(SYS_PROC_SHM_GET, (ewokos_addr_t)key, (ewokos_addr_t)size, (ewokos_addr_t)flag);
}

#ifdef __cplusplus
}
#endif


