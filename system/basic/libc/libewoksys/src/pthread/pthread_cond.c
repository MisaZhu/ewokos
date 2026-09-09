#include <pthread.h>
#include <ewoksys/semaphore.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

/*
 * Condition variables, built on the kernel's blocking semaphore.
 *
 * sem_wait is the waiter queue. A waiting task parks inside semaphore_enter()
 * and the kernel hands it a permit directly when a signal posts one, so a
 * condvar wait costs no cpu and wakes exactly the tasks that were signaled.
 *
 * sem_signal is an ordinary binary mutex guarding `waiters`.
 *
 * This replaces an implementation that had no way to sleep: it polled
 * semaphore_tryenter(sem_wait) with a SYS_YIELD in a forever loop, so every
 * waiting thread pinned a core at 100%, and it kept a separate `signaled`
 * counter that pthread_cond_signal() could bump with nobody parked to observe
 * it. The permit in sem_wait IS the signal now - there is nothing left to lose.
 *
 * `signaled` in pthread_cond_t is unused. The field stays because the struct is
 * embedded in pthread_rwlock_t and pthread_barrier_t, and changing its layout
 * would mean rebuilding every consumer for no gain.
 */

static inline int cond_state_lock(pthread_cond_t* cond) {
    return semaphore_enter(cond->sem_signal);
}

static inline void cond_state_unlock(pthread_cond_t* cond) {
    semaphore_quit(cond->sem_signal);
}

// Get current time in microseconds
static inline uint64_t get_time_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Convert timespec to microseconds
static inline uint64_t timespec_to_usec(const struct timespec *ts) {
    if(ts == NULL) return 0;
    return (uint64_t)ts->tv_sec * 1000000ULL + (ts->tv_nsec / 1000);
}

// Initialize condition variable
int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t *attr) {
    (void)attr;
    if(cond == NULL)
        return EINVAL;
    
    memset(cond, 0, sizeof(pthread_cond_t));
    
    /*
     * A counting semaphore starting at zero permits, not the binary one
     * semaphore_alloc() would give: pthread_cond_broadcast() posts one permit
     * per waiter, and a binary semaphore refuses every post after the first.
     */
    cond->sem_wait = semaphore_alloc_count(0);
    if(cond->sem_wait == 0)
        return ENOMEM;
    
    // Allocate signal notification semaphore
    cond->sem_signal = semaphore_alloc();
    if(cond->sem_signal == 0) {
        semaphore_free(cond->sem_wait);
        cond->sem_wait = 0;
        return ENOMEM;
    }
    
    cond->waiters = 0;
    cond->signaled = 0;
    
    return 0;
}

// Destroy condition variable
int pthread_cond_destroy(pthread_cond_t* cond) {
    if(cond == NULL)
        return EINVAL;
    
    /*
     * Freeing the semaphores releases anyone still parked on them: the kernel
     * purges the wait queue of a semaphore being torn down, so those tasks wake
     * with SEM_RES_ERROR instead of sleeping on a lock that no longer exists.
     */
    if(cond->sem_wait != 0) {
        semaphore_free(cond->sem_wait);
        cond->sem_wait = 0;
    }
    
    if(cond->sem_signal != 0) {
        semaphore_free(cond->sem_signal);
        cond->sem_signal = 0;
    }
    
    cond->waiters = 0;
    cond->signaled = 0;
    
    return 0;
}

// Internal implementation for condition variable wait
static int cond_wait_internal(pthread_cond_t* cond, pthread_mutex_t* mutex, 
                               const struct timespec *abstime, int timed) {
    if(cond == NULL || mutex == NULL)
        return EINVAL;
    
    if(cond->sem_wait == 0 || cond->sem_signal == 0)
        return EINVAL;
    
    /*
     * Register BEFORE releasing the mutex. If the mutex went first, a signal
     * arriving in the gap would see waiters == 0 and post no permit, and this
     * task would then park and sleep straight through the signal it was waiting
     * for.
     */
    if(cond_state_lock(cond) != 0)
        return EINVAL;
    cond->waiters++;
    cond_state_unlock(cond);
    
    // Release mutex
    int unlock_res = pthread_mutex_unlock(mutex);
    if(unlock_res != 0) {
        cond_state_lock(cond);
        if(cond->waiters > 0)
            cond->waiters--;
        cond_state_unlock(cond);
        return unlock_res;
    }
    
    int result = 0;
    
    if(timed && abstime != NULL) {
        /*
         * abstime is a wall-clock (CLOCK_REALTIME) instant, so the remaining
         * budget is measured against gettimeofday. semaphore_enter_timeout()
         * then tracks that budget on the monotonic kernel tic, which keeps a
         * clock adjustment from stretching or truncating the wait once it has
         * started.
         */
        uint64_t deadline = timespec_to_usec(abstime);
        uint64_t now = get_time_usec();
        if(now >= deadline) {
            result = ETIMEDOUT;
        } else {
            uint64_t remaining = deadline - now;
            if(remaining > 0xffffffffULL)
                remaining = 0xffffffffULL;
            int res = semaphore_enter_timeout(cond->sem_wait, (uint32_t)remaining);
            if(res == SEM_RES_ACQUIRED)
                result = 0;
            else if(res == SEM_RES_TIMEOUT)
                result = ETIMEDOUT;
            else
                result = EINVAL;
        }
    } else {
        // Infinite wait: parks in the kernel until signaled
        int res = semaphore_enter(cond->sem_wait);
        result = (res == SEM_RES_ACQUIRED) ? 0 : EINVAL;
    }
    
    /*
     * Settle the give-up path under the state lock.
     *
     * A waiter that did not get a permit has to undo its own registration,
     * because pthread_cond_signal() decrements `waiters` on behalf of the task
     * it wakes. Doing that unlocked races the signaler: it could decrement and
     * post in the gap, leaving a permit nobody consumes - which the NEXT waiter
     * would pick up as a spurious wake - while our own decrement underflows the
     * count the signaler is relying on.
     *
     * Under the lock the two are mutually exclusive, and the last-chance
     * semaphore_tryenter() picks up a permit that was posted for us after our
     * timed wait gave up: if it is there, we were signaled, so we take it and
     * leave `waiters` alone.
     */
    if(result != 0) {
        if(cond_state_lock(cond) == 0) {
            if(semaphore_tryenter(cond->sem_wait) == SEM_RES_ACQUIRED)
                result = 0;
            else if(cond->waiters > 0)
                cond->waiters--;
            cond_state_unlock(cond);
        }
    }
    
    // Re-acquire mutex
    int lock_res = pthread_mutex_lock(mutex);
    if(lock_res != 0 && result == 0) {
        result = lock_res;
    }
    
    return result;
}

// Wait on condition variable
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    return cond_wait_internal(cond, mutex, NULL, 0);
}

// Wait on condition variable with timeout
int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, 
                            const struct timespec *abstime) {
    if(abstime == NULL)
        return EINVAL;
    return cond_wait_internal(cond, mutex, abstime, 1);
}

// Signal one waiting thread
int pthread_cond_signal(pthread_cond_t* cond) {
    if(cond == NULL)
        return EINVAL;
    
    if(cond->sem_wait == 0 || cond->sem_signal == 0)
        return EINVAL;
    
    /*
     * The permit and the waiter count are updated together, under the state
     * lock, so a task giving up (see cond_wait_internal) can never disagree
     * with the signaler about who owns this wake.
     */
    cond_state_lock(cond);
    if(cond->waiters > 0) {
        cond->waiters--;
        semaphore_quit(cond->sem_wait);
    }
    cond_state_unlock(cond);
    
    return 0;
}

// Broadcast to all waiting threads
int pthread_cond_broadcast(pthread_cond_t* cond) {
    if(cond == NULL)
        return EINVAL;
    
    if(cond->sem_wait == 0 || cond->sem_signal == 0)
        return EINVAL;
    
    cond_state_lock(cond);
    int waiters = cond->waiters;
    cond->waiters = 0;
    /*
     * One permit per waiter, posted while the count is being cleared: a task
     * that arrives between here and the unlock registers itself afresh and is
     * not entitled to any of these permits.
     */
    for(int i = 0; i < waiters; i++)
        semaphore_quit(cond->sem_wait);
    cond_state_unlock(cond);
    
    return 0;
}
