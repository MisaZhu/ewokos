#include <pthread.h>
#include <ewoksys/thread.h>
#include <ewoksys/semaphore.h>

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
	if(mutex == NULL)
		return -1;
	return semaphore_quit(*mutex);
}