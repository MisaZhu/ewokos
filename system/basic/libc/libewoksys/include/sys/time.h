#ifndef EWOKOS_LIBC_SYS_TIME_H
#define EWOKOS_LIBC_SYS_TIME_H

#include <stdint.h>
#include <sys/types.h>

struct timeval {
	long tv_sec;
	long tv_usec;
};

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

struct timespec {
	long tv_sec;
	long tv_nsec;
};

struct utimbuf {
	time_t actime;
	time_t modtime;
};

#define ITIMER_REAL    0
#define ITIMER_VIRTUAL 1
#define ITIMER_PROF    2

struct itimerval {
	struct timeval it_interval;
	struct timeval it_value;
};

#ifdef __cplusplus
extern "C" {
#endif

int gettimeofday(struct timeval *tp, void *tzvp);
int settimeofday(const struct timeval *tp, const struct timezone *tzp);
int utimes(const char *path, const struct timeval times[2]);
int futimes(int fd, const struct timeval times[2]);
int utime(const char *path, const struct utimbuf *times);
int getitimer(int which, struct itimerval *curr_value);
int setitimer(int which, const struct itimerval *new_value,
              struct itimerval *old_value);

#ifdef __cplusplus
}
#endif

#endif
