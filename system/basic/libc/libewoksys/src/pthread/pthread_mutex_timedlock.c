#include <pthread.h>
#include <ewoksys/thread.h>
#include "pthread_mutex_common.h"
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

/*
 * Non-blocking acquire: a single CAS on the free word, no syscall. Returns 0
 * if we took it, EBUSY if anyone holds it (state HELD or WAIT). The word is
 * never left marked WAIT by a trylock, so a failed trylock costs nothing and
 * wakes nobody.
 */
int pthread_mutex_trylock(pthread_mutex_t* mutex) {
    if(mutex == NULL)
        return EINVAL;

    if(mtx_try_acquire((int32_t*)mutex))
        return 0;
    return EBUSY;
}

/*
 * Acquire with an absolute wall-clock deadline (CLOCK_REALTIME), as for
 * pthread_cond_timedwait. An uncontended lock is taken even when abstime is
 * already in the past, which is what POSIX asks for; only the contended path
 * measures the budget. The heavy lifting - park, re-contend, and the give-up
 * wake that keeps a fellow waiter from being stranded - is shared with
 * pthread_mutex_lock() in mtx_lock_contended().
 */
int pthread_mutex_timedlock(pthread_mutex_t* mutex, const struct timespec *abstime) {
    if(mutex == NULL || abstime == NULL)
        return EINVAL;

    int32_t* w = (int32_t*)mutex;

    if(mtx_try_acquire(w))
        return 0;

    return mtx_lock_contended(w, abstime, 1);
}
