#include <stdlib.h>
#include <string.h>
#include <ewoksys/syscall.h>

void* realloc(void* s, uint32_t new_size) {
	if(new_size == 0) {
		if(s != NULL)
			free(s);
		return NULL;
	}

	if(s == NULL)
		return malloc(new_size);
	
	uint32_t old_size = syscall1(SYS_MSIZE, (int32_t)s); 
	
	if(old_size >= new_size)
		return s;

	void* n = malloc(new_size);
	if(n != NULL)
		memcpy(n, s, old_size);
	free(s);
	return n;
}