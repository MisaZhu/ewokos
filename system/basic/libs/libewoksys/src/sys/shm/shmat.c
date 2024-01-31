#include <sys/shm.h>
#include <ewoksys/syscall.h>

void *shmat(int shmid, const void *addr, int flag) {
	(void)addr;
	void* p = (void*)syscall1(SYS_PROC_SHM_MAP, shmid);
	if(p == NULL)
		return NULL;
	return p;
}

