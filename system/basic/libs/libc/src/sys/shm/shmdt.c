#include <sys/shm.h>
#include <ewoksys/shm.h>

int shmdt(void* p) {
	if(p == NULL)
		return -1;
	return shm_dt(p);
}

