#include <pthread.h>
#include <ewoksys/thread.h>
#include <ewoksys/semaphore.h>
#include "pthread_mutex_common.h"

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t *attr) {
    (void)attr;
    if(mutex == NULL)
        return -1;
    /*
     * No kernel object is allocated here any more. The word starts FREE with
     * no park channel; the channel is created lazily the first time the mutex
     * is actually contended (see mtx_ensure_sem). This is also what makes
     * PTHREAD_MUTEX_INITIALIZER (all zero) a valid, ready-to-use mutex and
     * removes the old first-lock allocation race entirely.
     */
    *(volatile int32_t*)mutex = 0;
    return 0;
}
