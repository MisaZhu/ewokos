#include <time.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <ewoksys/kernel_tic.h>

int clock_gettime(clockid_t clock_id, struct timespec *tp) {
    struct timeval tv;

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
        uint64_t nsec;
        if (kernel_tic_nsec(&nsec) != 0) {
            errno = EIO;
            return -1;
        }
        tp->tv_sec = (long)(nsec / 1000000000ULL);
        tp->tv_nsec = (long)(nsec % 1000000000ULL);
        return 0;
    }

    errno = EINVAL;
    return -1;
}
