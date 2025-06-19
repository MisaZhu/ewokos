#include <sys/shm.h>
#include <ewoksys/syscall.h>


#ifdef __cplusplus
extern "C" {
#endif

void *shmat(int shmid, const void *addr, int flag) {
	(void)addr;
	void* p = (void*)syscall1(SYS_PROC_SHM_MAP, (ewokos_addr_t)shmid);
	if(p == NULL)
		return NULL;
	return p;
}


#ifdef __cplusplus
}
#endif
