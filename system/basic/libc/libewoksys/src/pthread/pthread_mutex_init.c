#include <pthread.h>
#include <ewoksys/thread.h>
#include <ewoksys/semaphore.h>

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t *attr) {
	(void)attr;
	if(mutex == NULL)
		return -1;
	*mutex = semaphore_alloc();
	if(*mutex == 0)
		return -1;
	return 0;
}