#include <stdlib.h>
#include <ewoksys/syscall.h>
#include <string.h>

void *calloc(size_t nmemb, size_t size) {
	void* p = malloc(nmemb*size);
	if(p != NULL)
		memset(p, 0, nmemb*size);
	return p;
}

