#include <pthread.h>
#include <ewoksys/semaphore.h>

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    if(mutex == NULL || *mutex == 0)
        return -1;
    semaphore_free(*mutex);
    /*
     * Clear the id. Left holding a freed one, the next pthread_mutex_lock()
     * would enter a semaphore the kernel has already handed back out to
     * somebody else - or, if the slot is still free, silently lock nothing.
     * Zeroing also lets the mutex be used again through the lazy-init path.
     */
    *mutex = 0;
    return 0;
}