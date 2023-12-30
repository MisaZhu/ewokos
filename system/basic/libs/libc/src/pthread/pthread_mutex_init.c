#include <pthread.h>
#include <ewoksys/thread.h>

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t *attr) {
	(void)attr;
	if(mutex == NULL)
		return -1;
	*mutex = semaphore_alloc();
	return 0;
}