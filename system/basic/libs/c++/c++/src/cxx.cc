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

/* operator new/delete are marked weak so that a link which pulls in BOTH
   libcxx.a and libewokstl.a does not fail with "multiple definition".
   libewokstl.a's new_delete.o provides a strict superset (array forms and
   the nothrow forms Qt needs) with strong symbols, so when both archives
   end up in the same link the strong definitions win and these weak ones
   are silently dropped.  A program that links -lcxx without -lewokstl still
   gets these definitions exactly as before - weak only affects which one
   wins when there is a choice, not whether the symbol is available.

   This is needed because ld extracts an archive member only when it
   resolves a currently-undefined symbol, and cxx_runtime.o (libewokstl.a's
   copy of the __cxa_* set) is not always extracted at -lewokstl time: if
   the C++ libraries between -lewokstl and -lcxx are the first to reference
   __cxa_pure_virtual or __cxa_guard_acquire, ld reaches -lcxx with those
   symbols still undefined, extracts cxx.o for them, and cxx.o drags its
   own operator new/delete in with it.  Making them weak removes the
   collision without changing which archive provides the __cxa_* set. */
__attribute__((weak)) void* operator new(size_t n) {
  void *p = malloc(n);
  return p;
}

__attribute__((weak)) void operator delete(void * p, size_t n) {
	(void)n;
  free(p);
}

__attribute__((weak)) void operator delete(void * p) {
  free(p);
}

