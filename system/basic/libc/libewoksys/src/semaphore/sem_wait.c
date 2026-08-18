#include <semaphore.h>
#include <ewoksys/semaphore.h>
#include <ewoksys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

static inline uint64_t sem_time_usec(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

/* Try to decrement the semaphore value once. Returns 1 on success. */
static int sem_try_acquire(sem_t *sem) {
	int got = 0;

	if(semaphore_enter(sem->lock) != 0)
		return -1;
	if(sem->value > 0) {
		sem->value--;
		got = 1;
	}
	semaphore_quit(sem->lock);
	return got;
}

int sem_trywait(sem_t *sem) {
	if(sem == NULL || sem->magic != SEM_MAGIC) {
		errno = EINVAL;
		return -1;
	}
	if(sem_try_acquire(sem) == 1)
		return 0;
	errno = EAGAIN;
	return -1;
}

int sem_wait(sem_t *sem) {
	if(sem == NULL || sem->magic != SEM_MAGIC) {
		errno = EINVAL;
		return -1;
	}
	while(1) {
		int res = sem_try_acquire(sem);
		if(res == 1)
			return 0;
		if(res < 0) {
			errno = EINVAL;
			return -1;
		}
		syscall0(SYS_YIELD);
	}
}

int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout) {
	uint64_t deadline;

	if(sem == NULL || sem->magic != SEM_MAGIC || abs_timeout == NULL ||
			abs_timeout->tv_nsec < 0 || abs_timeout->tv_nsec >= 1000000000L) {
		errno = EINVAL;
		return -1;
	}

	deadline = (uint64_t)abs_timeout->tv_sec * 1000000ULL +
		(uint64_t)(abs_timeout->tv_nsec / 1000);

	while(1) {
		int res = sem_try_acquire(sem);
		if(res == 1)
			return 0;
		if(res < 0) {
			errno = EINVAL;
			return -1;
		}

		uint64_t now = sem_time_usec();
		if(now >= deadline) {
			errno = ETIMEDOUT;
			return -1;
		}

		uint64_t remaining = deadline - now;
		if(remaining > 1000) {
			usleep(1000);
		} else if(remaining > 100) {
			usleep((useconds_t)(remaining / 2));
		} else {
			syscall0(SYS_YIELD);
		}
	}
}
