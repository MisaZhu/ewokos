#include <stdlib.h>

extern "C" {
	void __cxa_pure_virtual(void) {while(1);}
}

void* operator new(size_t n) {
  void *p = malloc(n);
  return p;
}

void operator delete(void * p, size_t n) {
	(void)n;
  free(p);
}

void operator delete(void * p) {
  free(p);
}

