#include <sys/syscall.h>
#include <sys/lockc.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


lock_t lock_new(void) {
	bool* lock = (bool*)malloc(sizeof(bool));
	if(lock == NULL)
		return -1;
	*lock = false;
	return (lock_t)lock;	
}

void lock_free(lock_t lock) {
	if(lock == 0)
		return;
	bool* p = (bool*)lock;
	free(p);
}

int lock_lock(lock_t lock) {
	while(1) {
		if(syscall1(SYS_LOCK, lock) == 0)
			break;
		sleep(0);
	}
	return 0;
}

void lock_unlock(lock_t lock) {
	syscall1(SYS_UNLOCK, lock);
}

#ifdef __cplusplus
}
#endif

