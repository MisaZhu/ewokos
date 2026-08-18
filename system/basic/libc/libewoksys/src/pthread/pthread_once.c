#include <pthread.h>
#include <ewoksys/syscall.h>
#include <stddef.h>

/*
 * pthread_once: serialize all once-controls through one recursive-aware
 * spin lock. Recursion detection is required because init_routine may call
 * pthread_once() on another control while the lock is held by this thread.
 */
static volatile int _once_locked = 0;
static volatile int _once_owner = 0;

static void once_lock(void) {
	int self = (int)pthread_self();

	if(_once_owner == self)
		return; /* recursive call from the same init routine */

	while(__sync_lock_test_and_set(&_once_locked, 1) != 0)
		syscall0(SYS_YIELD);
	_once_owner = self;
}

static void once_unlock(void) {
	int self = (int)pthread_self();

	if(_once_owner == self) {
		_once_owner = 0;
		__sync_lock_release(&_once_locked);
	}
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
	if(once_control == NULL || init_routine == NULL)
		return -1;

	if(*once_control != 0)
		return 0;

	once_lock();
	if(*once_control == 0) {
		init_routine();
		*once_control = 1;
	}
	once_unlock();
	return 0;
}
