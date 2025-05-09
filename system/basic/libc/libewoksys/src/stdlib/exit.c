#include <stdlib.h>
#include <ewoksys/syscall.h>

void exit(int status) {
	syscall1(SYS_EXIT, (ewokos_addr_t)status);
}

