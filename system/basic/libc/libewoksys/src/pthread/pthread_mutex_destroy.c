#include <pthread.h>
#include <ewoksys/semaphore.h>
#include "pthread_mutex_common.h"

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    if(mutex == NULL)
        return EINVAL;

    int32_t* w = (int32_t*)mutex;
    int sem = mtx_sem(mtx_load(w));

    /*
     * Reset the word first so a racing lock sees FREE, then release the park
     * channel if one was ever allocated. Freeing purges the kernel wait queue,
     * so any task still parked wakes with SEM_RES_ERROR instead of sleeping on
     * a channel that no longer exists. A mutex that was never contended has no
     * channel (sem == 0) and destroys to a no-op success - which is what makes
     * destroying a PTHREAD_MUTEX_INITIALIZER mutex, or pthread_rwlock's
     * `if(lock != 0)` guarded destroy, behave.
     */
    *(volatile int32_t*)w = 0;
    if(sem != 0)
        semaphore_free(sem);
    return 0;
}
