#include <sys/proclock.h>
#include <sys/syscall.h>

proc_lock_t proc_lock_new(void) {
	return (proc_lock_t)syscall0(SYS_LOCK_NEW);
}

void proc_lock_free(proc_lock_t lock) {
	syscall1(SYS_LOCK_FREE, lock);
}

void proc_lock(proc_lock_t lock) {
	while(1) {
		if(syscall1(SYS_LOCK, lock) == 0)
			break;
	}
}

void proc_unlock(proc_lock_t lock) {
	syscall1(SYS_UNLOCK, lock);
}

