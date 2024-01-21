#include <stdlib.h>

void free(void* ptr) {
	__mlock__();
	__free__(ptr);
	__munlock__();
}