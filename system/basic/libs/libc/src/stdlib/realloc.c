#include <stdlib.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <ewoksys/trunkmem.h>
#include <ewoksys/semaphore.h>

static int _sema_malloc = -1;
void __malloc_init() {
	_sema_malloc = semaphore_alloc();
}

void __malloc_close() {
	semaphore_free(_sema_malloc);
}

static void* realloc_(void* s, uint32_t new_size) {
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

void* realloc(void* s, uint32_t size) {
	semaphore_enter(_sema_malloc);
	void* ret = realloc_(s, size);
	semaphore_quit(_sema_malloc);
	return ret;
}