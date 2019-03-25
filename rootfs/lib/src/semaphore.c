#include <semaphore.h>
#include <syscall.h>
#include <unistd.h>

semaphore_t semaphore_alloc() {
	return syscall0(SYSCALL_SEMAPHORE_ALLOC);
}

void semaphore_free(semaphore_t s) {
	syscall1(SYSCALL_SEMAPHORE_FREE, s);
}

int32_t semaphore_lock(semaphore_t s) {
	while(syscall1(SYSCALL_SEMAPHORE_LOCK, s) < 0)
		yield();
	return 0;
}

int32_t semaphore_unlock(semaphore_t s) {
	return syscall1(SYSCALL_SEMAPHORE_UNLOCK, s);
}

