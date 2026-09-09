#include <semaphore.h>
#include <ewoksys/semaphore.h>
#include <sys/time.h>
#include <errno.h>
#include <stdint.h>

int sem_trywait(sem_t *sem) {
	if(sem == NULL || sem->magic != SEM_MAGIC) {
		errno = EINVAL;
		return -1;
	}
	int res = semaphore_tryenter(sem->ksem);
	if(res == SEM_RES_ACQUIRED)
		return 0;
	errno = (res == SEM_RES_OCCUPIED) ? EAGAIN : EINVAL;
	return -1;
}

int sem_wait(sem_t *sem) {
	if(sem == NULL || sem->magic != SEM_MAGIC) {
		errno = EINVAL;
		return -1;
	}
	/*
	 * Parks in the kernel until a permit is posted. This used to be a
	 * try-and-yield loop around the userspace count, which pinned a core for
	 * as long as the semaphore stayed empty.
	 */
	if(semaphore_enter(sem->ksem) == SEM_RES_ACQUIRED)
		return 0;
	errno = EINVAL;
	return -1;
}

int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout) {
	if(sem == NULL || sem->magic != SEM_MAGIC || abs_timeout == NULL ||
			abs_timeout->tv_nsec < 0 || abs_timeout->tv_nsec >= 1000000000L) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * A permit that is already available is taken even when abs_timeout is in
	 * the past; reporting ETIMEDOUT there would fail a wait nobody is
	 * contesting.
	 */
	int res = semaphore_tryenter(sem->ksem);
	if(res == SEM_RES_ACQUIRED)
		return 0;
	if(res != SEM_RES_OCCUPIED) {
		errno = EINVAL;
		return -1;
	}

	// abs_timeout is a wall-clock (CLOCK_REALTIME) instant
	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint64_t now = (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
	uint64_t deadline = (uint64_t)abs_timeout->tv_sec * 1000000ULL +
		(uint64_t)(abs_timeout->tv_nsec / 1000);
	if(now >= deadline) {
		errno = ETIMEDOUT;
		return -1;
	}

	/*
	 * What is left of the deadline, handed to the kernel as a duration. The
	 * kernel measures it against its own monotonic tic, so a wall-clock
	 * adjustment after this point cannot stretch or truncate the wait, and
	 * there is no 1ms usleep() probe waking us up in between.
	 */
	uint64_t remaining = deadline - now;
	if(remaining > 0xffffffffULL)
		remaining = 0xffffffffULL;

	res = semaphore_enter_timeout(sem->ksem, (uint32_t)remaining);
	if(res == SEM_RES_ACQUIRED)
		return 0;
	errno = (res == SEM_RES_TIMEOUT) ? ETIMEDOUT : EINVAL;
	return -1;
}
