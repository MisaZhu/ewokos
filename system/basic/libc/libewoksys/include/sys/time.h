#ifndef EWOKOS_LIBC_SYS_TIME_H
#define EWOKOS_LIBC_SYS_TIME_H

#include <stdint.h>

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

#ifdef __cplusplus
extern "C" {
#endif

int gettimeofday(struct timeval *tp, void *tzvp);

#ifdef __cplusplus
}
#endif

#endif
