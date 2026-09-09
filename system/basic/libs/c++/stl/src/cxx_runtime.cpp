/*
 * The C++ ABI runtime support backing libewokstl.a.
 *
 * These four functions used to come only from libcxx.a's cxx.o, which also
 * defines operator new and operator delete.  That was fine while libewokstl.a
 * had no allocators of its own, but src/new_delete.cpp added them for the
 * nothrow forms Qt needs, and the result is a link that wants both archives:
 * new_delete.o is extracted for operator new, cxx.o is extracted for
 * __cxa_guard_acquire, and cxx.o's own operator new then collides with the one
 * already defined - "multiple definition of `operator new(unsigned long)'".
 *
 * The fix is not to delete the allocators from either side, because programs
 * link these two archives in every combination: EWOK_LIB_X pulls in
 * -lewokstl, make.rule's EWOK_LIBC does not pull in -lcxx, and projects add
 * -lcxx themselves.  Whichever one lost its allocators would break some
 * combination that works today.
 *
 * Defining the __cxa_* set here instead makes cxx.o redundant rather than
 * conflicting.  ld only extracts an archive member that resolves a symbol which
 * is still undefined, and after this file every symbol cxx.o offers is already
 * defined by libewokstl.a - which EWOK_LIB_X puts ahead of -lcxx on every link
 * line in the tree.  cxx.o is therefore never extracted, the collision cannot
 * happen, and a program that links -lcxx without -lewokstl still gets cxx.o
 * exactly as it always did.
 *
 * The implementations are deliberately identical to cxx.cc's.  Changing the
 * semantics here would mean the two archives disagree, and which one a given
 * program ends up using would depend on its link line - a much worse property
 * than either choice on its own.
 *
 * On the guard functions specifically: they are not thread-safe, and that is a
 * property inherited from cxx.cc rather than introduced here.  __cxa_guard_*
 * protects function-local statics with non-trivial initializers, so two threads
 * reaching the same one for the first time would both initialize it.  Qt's own
 * shared statics (Q_GLOBAL_STATIC) use QAtomic and never come through here, and
 * making this correct would mean adding a lock or a compare-and-swap loop that
 * cxx.cc does not have.  That is a change worth making, but to both copies at
 * once and on its own, not as a side effect of a link fix.
 */

#include <stdint.h>

extern "C" {

/* Called through a pure virtual function pointer - a vtable slot that was never
   overridden, which in practice means a virtual call from a constructor or a
   destructor that ran past the point where the derived class existed.  There is
   no exception machinery to throw std::pure_virtual_call with (-fno-exceptions,
   no unwinder), so the only available answer is to stop.  Looping rather than
   returning keeps the caller from continuing with a garbage result, and gives a
   debugger something parked to look at. */
void __cxa_pure_virtual(void) {
	while (1) {
	}
}

/*
 * Guard variables for thread-safe static initialization.  The compiler emits a
 * __guard object per function-local static and brackets the initialization in
 * acquire/release.  The first byte is the "initialized" flag; the rest of the
 * eight bytes are the acquiring thread's id in a real implementation, which is
 * what makes a recursive initialization deadlock instead of corrupting.  Only
 * the flag is used here.
 *
 * Returns 1 if this caller should run the initializer, 0 if it is already done.
 */
int __cxa_guard_acquire(uint64_t* guard) {
	if (*((uint8_t*)guard) == 1)
		return 0;
	return 1;
}

/* Marks the initialization complete.  Called only after the initializer has
   run without unwinding. */
void __cxa_guard_release(uint64_t* guard) {
	*((uint8_t*)guard) = 1;
}

/* Called if the initializer throws.  There is nothing to throw with on this
   platform, so this is unreachable in practice; clearing the guard is what the
   ABI asks for, and it leaves the static retryable rather than permanently
   half-initialized. */
void __cxa_guard_abort(uint64_t* guard) {
	*guard = 0;
}

} /* extern "C" */
