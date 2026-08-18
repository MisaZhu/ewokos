#include <time.h>
#include <sys/errno.h>

int clock_getres(clockid_t clock_id, struct timespec *res) {
	if (res == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC) {
		errno = EINVAL;
		return -1;
	}

	/* Both clocks are derived from microsecond sources. */
	res->tv_sec = 0;
	res->tv_nsec = 1000L;
	return 0;
}
