#include <sys/shm.h>
#include <ewoksys/shm.h>

int *shmat(int shmid, const void *addr, int flag) {
	(void)addr;
	void* p = shm_at(shmid);
	if(p == NULL)
		return (int*)(-1);
	return (int*)p;
}

