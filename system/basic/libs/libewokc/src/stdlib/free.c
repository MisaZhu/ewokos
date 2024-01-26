#include <stdlib.h>

void free(void* ptr) {
	proc_global_lock();
	__free__(ptr);
	proc_global_unlock();
}