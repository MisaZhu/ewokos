#include <time.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <ewoksys/kernel_tic.h>

int clock_gettime(clockid_t clock_id, struct timespec *tp) {
	struct timeval tv;
	uint64_t usec;

	if (tp == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (clock_id == CLOCK_REALTIME) {
		if (gettimeofday(&tv, NULL) != 0) {
			return -1;
		}
		tp->tv_sec = tv.tv_sec;
		tp->tv_nsec = tv.tv_usec * 1000L;
		return 0;
	}

	if (clock_id == CLOCK_MONOTONIC) {
		if (kernel_tic(NULL, &usec) != 0) {
			errno = EIO;
			return -1;
		}
		tp->tv_sec = (long)(usec / 1000000ULL);
		tp->tv_nsec = (long)((usec % 1000000ULL) * 1000ULL);
		return 0;
	}

	errno = EINVAL;
	return -1;
}
