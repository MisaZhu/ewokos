#include <pthread.h>
#include <ewoksys/thread.h>
#include "pthread_mutex_common.h"

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    if(mutex == NULL)
        return -1;

    if(pthread_mutex_lazy_init(mutex) != 0)
        return -1;

    /*
     * Blocks in the kernel until the permit is ours. semaphore_enter() only
     * returns 0 (acquired) or -1 (the semaphore was freed under us), so there
     * is no "occupied" case left for the caller to retry.
     */
    return semaphore_enter(*mutex);
}
