#include <pthread.h>
#include <errno.h>

/* Thread cancellation is not supported; only the API surface exists. */

int pthread_cancel(pthread_t thread) {
	(void)thread;
	return ENOSYS;
}

int pthread_setcancelstate(int state, int *oldstate) {
	if(oldstate != NULL)
		*oldstate = PTHREAD_CANCEL_ENABLE;
	(void)state;
	return 0;
}

int pthread_setcanceltype(int type, int *oldtype) {
	if(oldtype != NULL)
		*oldtype = PTHREAD_CANCEL_DEFERRED;
	(void)type;
	return 0;
}

void pthread_testcancel(void) {
}
