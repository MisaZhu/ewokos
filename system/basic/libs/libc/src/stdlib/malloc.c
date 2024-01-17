#include <stdlib.h>
#include <ewoksys/syscall.h>

void* malloc(size_t size) {
	return (void*)syscall1(SYS_MALLOC, (int32_t)size);
}
