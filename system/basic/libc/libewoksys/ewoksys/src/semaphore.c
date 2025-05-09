#include <ewoksys/semaphore.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

int  semaphore_alloc(void) {
	return syscall0(SYS_SEMAPHORE_ALLOC);
}

void semaphore_free(int sem_id) {
	syscall1(SYS_SEMAPHORE_FREE, sem_id);
}

int  semaphore_enter(int sem_id) {
	int res = 0;
	while(true) {
		res = syscall1(SYS_SEMAPHORE_ENTER, (ewokos_addr_t)sem_id);
		if(res != -2)
			break;
	}
	return res;
}

int  semaphore_quit(int sem_id) {
	return syscall1(SYS_SEMAPHORE_QUIT, (ewokos_addr_t)sem_id);
}

#ifdef __cplusplus
}
#endif