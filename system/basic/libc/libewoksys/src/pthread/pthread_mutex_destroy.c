#include <pthread.h>
#include <ewoksys/semaphore.h>

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
	if(mutex == NULL || *mutex == 0)
		return -1;
	semaphore_free(*mutex);
	return 0;
}