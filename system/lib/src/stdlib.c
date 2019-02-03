#include <stdlib.h>
#include <syscall.h>

void* malloc(size_t size) {
	char* p = (char*)syscall1(SYSCALL_PMALLOC, size);
	return (void*)p;
}

void free(void* p) {
	syscall1(SYSCALL_PFREE, (int)p);
}
