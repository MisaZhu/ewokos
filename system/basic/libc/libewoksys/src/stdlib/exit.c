#include <stdlib.h>
#include <ewoksys/syscall.h>

int atexit(void (*func)(void)) {
	(void)func;
	return 0;
}

void exit(int status) {
	syscall1(SYS_EXIT, (ewokos_addr_t)status);
}
