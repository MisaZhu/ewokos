#include <sys/time.h>
#include <errno.h>

/*
 * The vfs layer has no protocol for updating file timestamps: accept the
 * calls without effect so that tools preserving timestamps still work.
 */
int utimes(const char *path, const struct timeval times[2]) {
    (void)times;
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int futimes(int fd, const struct timeval times[2]) {
    (void)times;
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }
    return 0;
}

int utime(const char *path, const struct utimbuf *times) {
    (void)times;
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

/* The kernel exposes no clock-setting interface. */
int settimeofday(const struct timeval *tp, const struct timezone *tzp) {
    (void)tp;
    (void)tzp;
    errno = ENOSYS;
    return -1;
}

int getitimer(int which, struct itimerval *curr_value) {
    if (curr_value == NULL ||
            (which != ITIMER_REAL && which != ITIMER_VIRTUAL &&
             which != ITIMER_PROF)) {
        errno = EINVAL;
        return -1;
    }
    curr_value->it_interval.tv_sec = 0;
    curr_value->it_interval.tv_usec = 0;
    curr_value->it_value.tv_sec = 0;
    curr_value->it_value.tv_usec = 0;
    return 0;
}

int setitimer(int which, const struct itimerval *new_value,
        struct itimerval *old_value) {
    if (new_value == NULL ||
            (which != ITIMER_REAL && which != ITIMER_VIRTUAL &&
             which != ITIMER_PROF)) {
        errno = EINVAL;
        return -1;
    }
    if (old_value != NULL) {
        old_value->it_interval.tv_sec = 0;
        old_value->it_interval.tv_usec = 0;
        old_value->it_value.tv_sec = 0;
        old_value->it_value.tv_usec = 0;
    }
    /* Arming a timer is not supported; disarming is accepted. */
    if (new_value->it_value.tv_sec != 0 || new_value->it_value.tv_usec != 0) {
        errno = ENOSYS;
        return -1;
    }
    return 0;
}
