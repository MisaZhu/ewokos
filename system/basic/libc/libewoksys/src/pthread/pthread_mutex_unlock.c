#include <pthread.h>
#include <ewoksys/thread.h>
#include <ewoksys/semaphore.h>
#include "pthread_mutex_common.h"

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    if(mutex == NULL)
        return EINVAL;

    int32_t* w = (int32_t*)mutex;

    while(1) {
        int32_t cur = mtx_load(w);
        int st = mtx_state(cur);

        if(st == MTX_FREE)
            return EINVAL;   /* not locked (mirrors the old binary-sem refusal) */

        if(st == MTX_HELD) {
            /* No waiters: clear the state bits, keep the sem id. No syscall. */
            if(mtx_cas_state(w, cur, MTX_FREE))
                return 0;
            continue;
        }

        /*
         * Waiters present: release the word, then post one wake. Read the sem
         * id from the same word we are CASing - state == WAIT implies a waiter
         * published it before announcing itself, and the whole-word CAS fails
         * (and retries) if a new waiter publishes concurrently, so the id we
         * post to is always the live one.
         */
        int sem = mtx_sem(cur);
        if(mtx_cas_state(w, cur, MTX_FREE)) {
            if(sem != 0)
                semaphore_quit(sem);   /* wakes the oldest waiter */
            return 0;
        }
        /* CAS failed; retry against the fresh word */
    }
}
