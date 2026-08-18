#include <time.h>
#include <sys/errno.h>

/* The kernel exposes no clock-setting interface. */
int clock_settime(clockid_t clock_id, const struct timespec *tp) {
	(void)clock_id;
	(void)tp;
	errno = ENOSYS;
	return -1;
}

int clock_nanosleep(clockid_t clock_id, int flags,
		const struct timespec *request, struct timespec *remain) {
	struct timespec now;

	if (request == NULL || request->tv_sec < 0 ||
			request->tv_nsec < 0 || request->tv_nsec >= 1000000000L)
		return EINVAL;

	if (remain != NULL) {
		remain->tv_sec = 0;
		remain->tv_nsec = 0;
	}

	if ((flags & TIMER_ABSTIME) != 0) {
		if (clock_gettime(clock_id, &now) != 0)
			return EINVAL;
		struct timespec rel;
		rel.tv_sec = request->tv_sec - now.tv_sec;
		rel.tv_nsec = request->tv_nsec - now.tv_nsec;
		if (rel.tv_nsec < 0) {
			rel.tv_sec--;
			rel.tv_nsec += 1000000000L;
		}
		if (rel.tv_sec < 0)
			return 0; /* deadline already passed */
		if (nanosleep(&rel, NULL) != 0)
			return EINVAL;
		return 0;
	}

	if (nanosleep(request, NULL) != 0)
		return EINVAL;
	return 0;
}
