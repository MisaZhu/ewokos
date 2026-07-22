#include <time.h>
#include <unistd.h>
#include <sys/errno.h>

int nanosleep(const struct timespec *req, struct timespec *rem) {
	unsigned long long total_usec;

	if (req == NULL || req->tv_sec < 0 || req->tv_nsec < 0 || req->tv_nsec >= 1000000000L) {
		errno = EINVAL;
		return -1;
	}

	total_usec = (unsigned long long)req->tv_sec * 1000000ULL +
		(unsigned long long)req->tv_nsec / 1000ULL;
	if (rem != NULL) {
		rem->tv_sec = 0;
		rem->tv_nsec = 0;
	}

	if (total_usec == 0) {
		return 0;
	}

	if (total_usec <= 0xffffffffULL) {
		return usleep((useconds_t)total_usec);
	}

	while (total_usec > 1000000ULL) {
		sleep(1);
		total_usec -= 1000000ULL;
	}
	return usleep((useconds_t)total_usec);
}
