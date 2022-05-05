#include <pthread.h>
#include <sys/thread.h>

int pthread_mutex_lock(pthread_mutex_t* mutex) {
	if(mutex == NULL)
		return -1;
	thread_safe_set(mutex, 1);
	return 0;
}