#include <semaphore.h>
#include <ewoksys/semaphore.h>
#include <errno.h>

int sem_post(sem_t *sem) {
	if(sem == NULL || sem->magic != SEM_MAGIC) {
		errno = EINVAL;
		return -1;
	}

	if(semaphore_enter(sem->lock) != 0) {
		errno = EINVAL;
		return -1;
	}
	if(sem->value >= SEM_VALUE_MAX) {
		semaphore_quit(sem->lock);
		errno = EOVERFLOW;
		return -1;
	}
	sem->value++;
	semaphore_quit(sem->lock);
	return 0;
}
