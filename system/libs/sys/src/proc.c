#include <sys/proc.h>
#include <sys/syscall.h>
#include <unistd.h>

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

int proc_ping(int pid) {
	return syscall1(SYS_PROC_PING, (int32_t)pid);
}

void proc_ready_ping(void) {
	syscall0(SYS_PROC_READY_PING);
}

void proc_wait_ready(int pid) {
	while(1) {
		if(proc_ping(pid) == 0)
			break;
		usleep(10000);
	}
}

