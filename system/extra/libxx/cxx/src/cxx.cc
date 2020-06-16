extern "C" {
	#include <stdlib.h>

	void __cxa_pure_virtual(void) {while(1);}
}

void* operator new(size_t n) {
  void *p = malloc(n);
  return p;
}

void operator delete(void * p) {
  free(p);
}

