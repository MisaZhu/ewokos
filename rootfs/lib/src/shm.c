#include <shm.h>
#include <syscall.h>

int32_t shmalloc(uint32_t size) {
	return syscall1(SYSCALL_SHM_ALLOC, (int32_t)size);	
}

void shmfree(int32_t id) {
	syscall1(SYSCALL_SHM_FREE, id);	
}

void* shmMap(int32_t id) {
	return (void*)syscall1(SYSCALL_SHM_MAP, id);
}

int32_t shmUnmap(int32_t id) {
	return syscall1(SYSCALL_SHM_UNMAP, id);
}
