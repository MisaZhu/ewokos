#include <sys/proc.h>
#include <sys/syscall.h>
#include <unistd.h>

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

