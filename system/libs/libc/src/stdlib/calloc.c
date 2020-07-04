#include <stdlib.h>
#include <sys/syscall.h>
#include <string.h>

void *calloc(size_t nmemb, size_t size) {
	void* p = (void*)syscall1(SYS_MALLOC, (int32_t)(nmemb*size));
	if(p != NULL)
		memset(p, 0, size);
	return p;
}

