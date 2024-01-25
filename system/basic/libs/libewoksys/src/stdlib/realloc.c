#include <stdlib.h>
#include <string.h>
#include <ewoksys/ewokdef.h>

static void* realloc_(void* s, size_t new_size) {
	if(new_size == 0) {
		if(s != NULL)
			__free__(s);
		return NULL;
	}

	if(s == NULL)
		return __malloc__(new_size);
	
	uint32_t old_size = 0;
	old_size = __size__(s); 
	
	if(old_size >= new_size)
		return s;

	void* n = __malloc__(new_size);
	if(n != NULL)
		memcpy(n, s, old_size);
	__free__(s);
	return n;
}

void* realloc(void* s, size_t size) {
	__mlock__();
	void* ret = realloc_(s, size);
	__munlock__();
	return ret;
}