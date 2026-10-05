#include <time.h>
#include <unistd.h>
#include <sys/errno.h>
#include <ewoksys/proc.h>

/*
 * tv_sec is a long, and tv_sec*1e9 overflows 64 bits past ~584 years. Saturate
 * rather than wrap: a wrapped total would silently turn a long sleep into a
 * short one. 1e9 seconds (~31 years) is the largest value that cannot overflow.
 */
#define NANOSLEEP_SATURATE_SEC 1000000000ULL
#define NANOSLEEP_MAX_NSEC     0xffffffffffffffffULL

int nanosleep(const struct timespec *req, struct timespec *rem) {
    unsigned long long total_nsec;
    unsigned long long total_usec;

    if (req == NULL || req->tv_sec < 0 || req->tv_nsec < 0 || req->tv_nsec >= 1000000000L) {
        errno = EINVAL;
        return -1;
    }

    if (rem != NULL) {
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }

    if ((unsigned long long)req->tv_sec >= NANOSLEEP_SATURATE_SEC)
        total_nsec = NANOSLEEP_MAX_NSEC;
    else
        total_nsec = (unsigned long long)req->tv_sec * 1000000000ULL +
            (unsigned long long)req->tv_nsec;

    if (total_nsec == 0) {
        return 0;
    }

    /*
     * Preferred path: sub-tick resolution. This is what makes nanosleep() mean
     * what it says - the plain usleep() below resolves on the scheduler tick,
     * ~976us at the default timer_freq=1024, and also throws away everything
     * below a microsecond.
     */
    if (total_nsec != NANOSLEEP_MAX_NSEC && proc_nsleep_precise(total_nsec) == 0) {
        return 0;
    }

    /* Coarse fallback: no user-readable counter on this platform (lego.ev3,
       x86), or the request was saturated above. */
    total_usec = total_nsec / 1000ULL;
    if (total_usec <= 0xffffffffULL) {
        return usleep((useconds_t)total_usec);
    }

    while (total_usec > 1000000ULL) {
        sleep(1);
        total_usec -= 1000000ULL;
    }
    return usleep((useconds_t)total_usec);
}
