#include <shm.h>
#include <syscall.h>

int32_t shm_alloc(uint32_t size) {
	return syscall1(SYSCALL_SHM_ALLOC, (int32_t)size);	
}

void shm_free(int32_t id) {
	syscall1(SYSCALL_SHM_FREE, id);	
}

void* shm_map(int32_t id) {
	return (void*)syscall1(SYSCALL_SHM_MAP, id);
}

int32_t shm_unmap(int32_t id) {
	return syscall1(SYSCALL_SHM_UNMAP, id);
}
