#include <sys/shm.h>
#include <ewoksys/shm.h>

int shmget(key_t key, int size, int flag) {
	return shm_get(key, size, flag);
}


