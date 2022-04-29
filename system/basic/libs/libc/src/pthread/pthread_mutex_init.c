#include <pthread.h>
#include <sys/thread.h>

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t *attr) {
	(void)attr;
	if(mutex == NULL)
		return -1;
	*mutex = 0;
	return 0;
}