#include <pthread.h>
#include <unistd.h>

int pthread_join(pthread_t thread, void **retval) {
	(void)thread;
	(void)retval;
	/* EwokOS doesn't have thread_join, so we just return success.
	 * The thread will clean up itself when it exits. */
	return 0;
}
