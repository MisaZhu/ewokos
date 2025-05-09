#include <sys/shm.h>
#include <ewoksys/syscall.h>

int shmdt(void* p) {
	if(p == NULL)
		return -1;
	return syscall1(SYS_PROC_SHM_UNMAP, (ewokos_addr_t)p);
}

