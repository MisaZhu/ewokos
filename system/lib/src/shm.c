#include <shm.h>
#include <syscall.h>

void* shmalloc(uint32_t size) {
	return (void*)syscall1(SYSCALL_SHM_ALLOC, (int32_t)size);	
}

int32_t shmMap(void* p) {
	return syscall1(SYSCALL_SHM_MAP, (int32_t)p);
}

int32_t shmUnmap(void* p) {
	return syscall1(SYSCALL_SHM_UNMAP, (int32_t)p);
}
