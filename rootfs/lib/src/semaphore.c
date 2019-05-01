#include <semaphore.h>
#include <syscall.h>
#include <unistd.h>

int32_t semaphore_init(semaphore_t* s) {
	return syscall1(SYSCALL_SEMAPHORE_INIT, (int32_t)s);
}

int32_t semaphore_close(semaphore_t* s) {
	return syscall1(SYSCALL_SEMAPHORE_CLOSE, (int32_t)s);
}

int32_t semaphore_lock(semaphore_t* s) {
	//return syscall1(SYSCALL_SEMAPHORE_LOCK, (int32_t)s);
	while(syscall1(SYSCALL_SEMAPHORE_LOCK, (int32_t)s) < 0)
		sleep(0);
	return 0;
}

int32_t semaphore_unlock(semaphore_t* s) {
	return syscall1(SYSCALL_SEMAPHORE_UNLOCK, (int32_t)s);
}

