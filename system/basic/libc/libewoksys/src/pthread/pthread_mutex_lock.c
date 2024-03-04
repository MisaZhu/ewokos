#include <pthread.h>
#include <ewoksys/thread.h>
#include <ewoksys/semaphore.h>

int pthread_mutex_lock(pthread_mutex_t* mutex) {
	if(mutex == NULL)
		return -1;

	if(*mutex == 0)
		*mutex = semaphore_alloc();
	if(*mutex == 0)
		return -1;

	return semaphore_enter(*mutex);
}