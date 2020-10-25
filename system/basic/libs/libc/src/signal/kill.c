#include <signal.h>
#include <sys/syscall.h>

int kill(int pid, int sig) {
	return syscall2(SYS_SIGNAL, pid, sig);
}
