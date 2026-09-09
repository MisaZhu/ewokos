#include <semaphore.h>
#include <ewoksys/semaphore.h>
#include <errno.h>
#include <stddef.h>
#include <stdarg.h>

int sem_init(sem_t *sem, int pshared, unsigned int value) {
	(void)pshared;
	if(sem == NULL || value > (unsigned int)SEM_VALUE_MAX) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * A counting semaphore with `value` permits already available. The kernel
	 * leaves the ceiling uncapped for this shape, so sem_post() may raise the
	 * count from a task that never waited.
	 */
	sem->ksem = semaphore_alloc_count((int)value);
	if(sem->ksem == 0) {
		errno = ENOMEM;
		return -1;
	}
	sem->magic = SEM_MAGIC;
	return 0;
}

int sem_destroy(sem_t *sem) {
	if(sem == NULL || sem->magic != SEM_MAGIC) {
		errno = EINVAL;
		return -1;
	}
	/*
	 * Freeing releases anyone parked in sem_wait(): the kernel purges the wait
	 * queue of a semaphore being torn down, so those tasks wake with an error
	 * instead of sleeping on a semaphore that no longer exists.
	 */
	if(sem->ksem != 0)
		semaphore_free(sem->ksem);
	sem->ksem = 0;
	sem->magic = 0;
	return 0;
}

int sem_getvalue(sem_t *sem, int *sval) {
	if(sem == NULL || sem->magic != SEM_MAGIC || sval == NULL) {
		errno = EINVAL;
		return -1;
	}
	int count = semaphore_get_count(sem->ksem);
	if(count < 0) {
		errno = EINVAL;
		return -1;
	}
	*sval = count;
	return 0;
}

/* Named semaphores are not supported. */
sem_t *sem_open(const char *name, int oflag, ...) {
	(void)name;
	(void)oflag;
	errno = ENOSYS;
	return SEM_FAILED;
}

int sem_close(sem_t *sem) {
	(void)sem;
	errno = ENOSYS;
	return -1;
}

int sem_unlink(const char *name) {
	(void)name;
	errno = ENOSYS;
	return -1;
}
