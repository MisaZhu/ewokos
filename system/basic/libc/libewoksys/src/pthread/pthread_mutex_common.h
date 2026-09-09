#ifndef EWOKOS_PTHREAD_MUTEX_COMMON_H
#define EWOKOS_PTHREAD_MUTEX_COMMON_H

#include <pthread.h>
#include <ewoksys/semaphore.h>

/*
 * Install a kernel semaphore into a zero-initialized pthread_mutex_t.
 *
 * POSIX lets a mutex be used straight from static zero initialization
 * (PTHREAD_MUTEX_INITIALIZER), and pthread_mutex_t here is nothing but the
 * semaphore id, so the id has to be allocated on first use. Doing that with a
 * plain "if(*mutex == 0) *mutex = semaphore_alloc()" races: two threads
 * reaching the first lock together each allocate a different id, the loser's id
 * is leaked, and for a window the two guard the same data with different locks.
 * That is the race proc_malloc_lock_prepare() and netd's task_init() both work
 * around by initializing their mutexes eagerly while still single-threaded.
 *
 * Allocating first and then racing to publish with a CAS leaves exactly one
 * winner; the loser frees the id it allocated. Both threads are in the same
 * proc, which is what semaphore_free() checks ownership against, so the free
 * always succeeds. Returns 0 once *mutex holds a valid id.
 *
 * Callers that can, should still pthread_mutex_init() explicitly: this closes
 * the race but cannot make an already-shared first-lock cheap.
 */
static inline int pthread_mutex_lazy_init(pthread_mutex_t* mutex) {
    if(*mutex != 0)
        return 0;

    int32_t id = (int32_t)semaphore_alloc();
    if(id == 0)
        return -1;

    if(!__sync_bool_compare_and_swap(mutex, (int32_t)0, id))
        semaphore_free(id);

    return (*mutex != 0) ? 0 : -1;
}

#endif
