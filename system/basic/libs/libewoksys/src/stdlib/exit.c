#include <stdlib.h>
#include <ewoksys/syscall.h>

void exit(int status) {
	syscall1(SYS_EXIT, (int32_t)status);
}

