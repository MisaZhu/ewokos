#include <ewoksys/shm.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif


int32_t shm_get(int32_t id, unsigned int size, int flag) {
	return syscall3(SYS_PROC_SHM_GET, id, size, flag);
}

void* shm_at(int32_t id) {
	return (void*)syscall1(SYS_PROC_SHM_MAP, id);
}

int shm_dt(void* p) {
	return syscall1(SYS_PROC_SHM_UNMAP, (int32_t)p);
}

#ifdef __cplusplus
}
#endif

