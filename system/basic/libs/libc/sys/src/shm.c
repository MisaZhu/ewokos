#include <sys/shm.h>
#include <sys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif


void* shm_alloc(unsigned int size, int flag) {
	void* p = (void*)syscall2(SYS_PROC_SHM_ALLOC, size, flag);
	if(p == NULL)
		return NULL;
	return shm_map(p);
}

void* shm_map(void* p) {
	return (void*)syscall1(SYS_PROC_SHM_MAP, p);
}

int shm_unmap(void* p) {
	return syscall1(SYS_PROC_SHM_UNMAP, p);
}

#ifdef __cplusplus
}
#endif

