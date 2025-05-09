#include <signal.h>
#include <ewoksys/syscall.h>

int kill(int pid, int sig) {
	return syscall2(SYS_SIGNAL, (ewokos_addr_t)pid, (ewokos_addr_t)sig);
}
