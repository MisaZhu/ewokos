#include <stdlib.h>
#include <ewoksys/syscall.h>

void free(void* ptr) {
	syscall1(SYS_FREE, (int32_t)ptr);
}
