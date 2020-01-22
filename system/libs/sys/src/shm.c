#include <sys/shm.h>
#include <sys/syscall.h>

int shm_alloc(unsigned int size, int flag) {
	return syscall2(SYS_PROC_SHM_ALLOC, size, flag);
}

void* shm_map(int shmid) {
	return (void*)syscall1(SYS_PROC_SHM_MAP, shmid);
}

int shm_unmap(int shmid) {
	return syscall1(SYS_PROC_SHM_UNMAP, shmid);
}
