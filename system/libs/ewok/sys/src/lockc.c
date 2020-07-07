#include <sys/syscall.h>
#include <sys/lockc.h>
#include <stdlib.h>

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

static int lock_lock_raw(lock_t lock) {
	if(lock == 0)
		return 0;
	bool* locked = (bool*)lock;
	if(*locked)
		return -1;
	syscall2(SYS_SAFE_SET, lock, true);
	return 0;
}

int lock_lock(lock_t lock) {
	while(1) {
		if(lock_lock_raw(lock) == 0)
			break;
		sleep(0);
	}
	return 0;
}

void lock_unlock(lock_t lock) {
	if(lock == 0)
		return 0;
	syscall2(SYS_SAFE_SET, lock, false);
}

#ifdef __cplusplus
}
#endif

