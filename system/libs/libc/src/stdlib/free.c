#include <stdlib.h>
#include <sys/syscall.h>

void free(void* ptr) {
	syscall1(SYS_FREE, (int32_t)ptr);
}

