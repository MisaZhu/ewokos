#include <sys/syscall.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


uint32_t lock_new(void) {
	bool* lock = (bool*)malloc(sizeof(bool));
	if(lock == NULL)
		return -1;
	*lock = false;
	return (uint32_t)lock;	
}

void lock_free(uint32_t lock) {
	if(lock == 0)
		return;
	bool* p = (bool*)lock;
	free(p);
}

static int lock_lock_raw(uint32_t lock) {
	if(lock == 0)
		return 0;
	bool* locked = (bool*)lock;
	if(*locked)
		return -1;
	syscall2(SYS_SAFE_SET, lock, true);
	return 0;
}

int lock_lock(uint32_t lock) {
	while(1) {
		if(lock_lock_raw(lock) == 0)
			break;
		sleep(0);
	}
	return 0;
}

void lock_unlock(uint32_t lock) {
	if(lock == 0)
		return 0;
	syscall2(SYS_SAFE_SET, lock, false);
}

#ifdef __cplusplus
}
#endif

