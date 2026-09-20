#include <pthread.h>
#include <ewoksys/thread.h>
#include "pthread_mutex_common.h"

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    if(mutex == NULL)
        return EINVAL;

    int32_t* w = (int32_t*)mutex;

    /*
     * Uncontended case: one CAS, no syscall. This is the whole point of the
     * redesign - the old code turned every lock into a SYS_SEMAPHORE_ENTER
     * that also took the kernel's global proc lock, even with nobody else
     * anywhere near the mutex.
     */
    if(mtx_try_acquire(w))
        return 0;

    return mtx_lock_contended(w, NULL, 0);
}
