#include <stdlib.h>
#include <sys/syscall.h>

void exit(int status) {
	syscall2(SYS_EXIT, -1, (int32_t)status);
}

