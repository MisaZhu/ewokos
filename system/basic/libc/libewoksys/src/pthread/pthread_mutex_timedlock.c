#include <pthread.h>
#include <ewoksys/thread.h>
#include "pthread_mutex_common.h"
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

// Get current time in microseconds
static inline uint64_t get_time_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Try to acquire mutex lock (non-blocking)
int pthread_mutex_trylock(pthread_mutex_t* mutex) {
    if(mutex == NULL)
        return EINVAL;

    if(pthread_mutex_lazy_init(mutex) != 0)
        return ENOMEM;

    /*
     * semaphore_tryenter() is genuinely non-blocking now. It used to be an
     * alias for the blocking enter in the kernel, so trylock blocked - the
     * opposite of what it promises - and EBUSY could never be returned.
     */
    int res = semaphore_tryenter(*mutex);
    if(res == SEM_RES_OCCUPIED)
        return EBUSY;
    return (res == SEM_RES_ACQUIRED) ? 0 : EINVAL;
}

// Acquire mutex lock with timeout
int pthread_mutex_timedlock(pthread_mutex_t* mutex, const struct timespec *abstime) {
    if(mutex == NULL || abstime == NULL)
        return EINVAL;

    if(pthread_mutex_lazy_init(mutex) != 0)
        return ENOMEM;

    /*
     * An uncontended lock is taken even when abstime is already in the past,
     * which is what POSIX asks for; checking the deadline first would report
     * ETIMEDOUT for a mutex nobody else holds.
     */
    int res = semaphore_tryenter(*mutex);
    if(res == SEM_RES_ACQUIRED)
        return 0;
    if(res != SEM_RES_OCCUPIED)
        return EINVAL;

    // abstime is a wall-clock (CLOCK_REALTIME) instant, as for pthread_cond_timedwait
    uint64_t deadline = (uint64_t)abstime->tv_sec * 1000000ULL +
                        (uint64_t)(abstime->tv_nsec / 1000);
    uint64_t now = get_time_usec();
    if(now >= deadline)
        return ETIMEDOUT;

    /*
     * Park in the kernel for what is left of the deadline instead of probing it
     * with usleep(): the old loop woke every millisecond to retry, so a
     * contended timed lock cost a syscall and a reschedule per millisecond per
     * waiter, and the retry could still lose the permit to a thread that
     * arrived after the sleep started.
     */
    uint64_t remaining = deadline - now;
    if(remaining > 0xffffffffULL)
        remaining = 0xffffffffULL;

    res = semaphore_enter_timeout(*mutex, (uint32_t)remaining);
    if(res == SEM_RES_ACQUIRED)
        return 0;
    if(res == SEM_RES_TIMEOUT)
        return ETIMEDOUT;
    return EINVAL;
}
