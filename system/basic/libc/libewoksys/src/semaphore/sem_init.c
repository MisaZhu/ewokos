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

	sem->lock = semaphore_alloc();
	if(sem->lock == 0) {
		errno = ENOMEM;
		return -1;
	}
	sem->value = (int32_t)value;
	sem->magic = SEM_MAGIC;
	return 0;
}

int sem_destroy(sem_t *sem) {
	if(sem == NULL || sem->magic != SEM_MAGIC) {
		errno = EINVAL;
		return -1;
	}
	if(sem->lock != 0)
		semaphore_free(sem->lock);
	sem->lock = 0;
	sem->value = 0;
	sem->magic = 0;
	return 0;
}

int sem_getvalue(sem_t *sem, int *sval) {
	if(sem == NULL || sem->magic != SEM_MAGIC || sval == NULL) {
		errno = EINVAL;
		return -1;
	}
	semaphore_enter(sem->lock);
	*sval = sem->value;
	semaphore_quit(sem->lock);
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
