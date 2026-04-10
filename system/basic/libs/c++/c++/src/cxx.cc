#include <stdlib.h>
#include <stdint.h>

extern "C" {
	void __cxa_pure_virtual(void) {while(1);}

	// Guard variables for thread-safe static initialization
	// In a single-threaded embedded environment, we just need to track if initialization is done
	
	// __cxa_guard_acquire - returns 1 if initialization should proceed, 0 if already initialized
	int __cxa_guard_acquire(uint64_t *guard) {
		// Check if already initialized (first byte is 1)
		if (*((uint8_t*)guard) == 1) {
			return 0;  // Already initialized
		}
		return 1;  // Needs initialization
	}

	// __cxa_guard_release - marks initialization as complete
	void __cxa_guard_release(uint64_t *guard) {
		// Mark as initialized by setting first byte to 1
		*((uint8_t*)guard) = 1;
	}

	// __cxa_guard_abort - called if initialization throws an exception
	void __cxa_guard_abort(uint64_t *guard) {
		// In embedded environment without exceptions, this shouldn't be called
		// But we clear the guard to allow retry
		*guard = 0;
	}
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

