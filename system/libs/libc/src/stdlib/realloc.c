#include <stdlib.h>
#include <sys/syscall.h>

void* realloc(void* s, uint32_t new_size) {
	return (void*)syscall2(SYS_REALLOC, (int32_t)s, (int32_t)new_size);
}

