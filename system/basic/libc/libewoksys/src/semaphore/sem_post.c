#include <semaphore.h>
#include <ewoksys/semaphore.h>
#include <errno.h>

int sem_post(sem_t *sem) {
	if(sem == NULL || sem->magic != SEM_MAGIC) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * Adds a permit and hands it straight to the oldest waiter, which the
	 * kernel wakes. Legal from a task that never waited - that is why the
	 * semaphore behind a sem_t is the counting shape and not the binary one,
	 * whose ceiling would refuse every post that has no matching wait.
	 */
	if(semaphore_quit(sem->ksem) == 0)
		return 0;

	/*
	 * quit() refuses either because the count is already at the ceiling or
	 * because the semaphore was freed underneath us, and does not say which,
	 * so ask. A count can only sit at SEM_VALUE_MAX if it was posted that
	 * many times, which makes EOVERFLOW the honest answer there.
	 */
	errno = (semaphore_get_count(sem->ksem) >= SEM_VALUE_MAX) ? EOVERFLOW : EINVAL;
	return -1;
}
