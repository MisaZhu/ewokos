/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software; however, the code below was
  adapted for EwokOS.
*/
#include "../../SDL_internal.h"

/* An implementation of condition variables using semaphores and mutexes */
/*
   This implementation borrows heavily from the BeOS condition variable
   implementation, written by Christopher Tate and Owen Smith.  Thanks!

   EwokOS note: the bookkeeping (waiting/signals) is protected by an
   atomic CAS and the wait/done queues are EwokOS kernel semaphores, so
   creating a cond no longer creates SDL mutexes/semaphores (which used
   to recurse infinitely: CreateSemaphore -> CreateCond -> CreateMutex
   -> CreateSemaphore).
 */

#include "SDL_thread.h"
#include "SDL_timer.h"

#include <ewoksys/semaphore.h>

struct SDL_cond
{
    int waiting;
    int signals;
    int wait_sem;
    int wait_done;
};

static int cond_cas(int *var, int oldval, int newval)
{
    return __sync_bool_compare_and_swap(var, oldval, newval);
}

/* Create a condition variable */
SDL_cond *
SDL_CreateCond(void)
{
    SDL_cond *cond;

    cond = (SDL_cond *) SDL_malloc(sizeof(SDL_cond));
    if (cond) {
        /* counting semaphores starting at zero: semaphore_alloc() hands out a
           binary mutex whose single permit is already free, which would let
           the first SDL_CondWait fall straight through and would cap
           SDL_CondBroadcast at one post. */
        cond->wait_sem = semaphore_alloc_count(0);
        cond->wait_done = semaphore_alloc_count(0);
        cond->waiting = cond->signals = 0;
        if (cond->wait_sem <= 0 || cond->wait_done <= 0) {
            SDL_DestroyCond(cond);
            cond = NULL;
        }
    } else {
        SDL_OutOfMemory();
    }
    return (cond);
}

/* Destroy a condition variable */
void
SDL_DestroyCond(SDL_cond * cond)
{
    if (cond) {
        if (cond->wait_sem > 0) {
            semaphore_free(cond->wait_sem);
        }
        if (cond->wait_done > 0) {
            semaphore_free(cond->wait_done);
        }
        SDL_free(cond);
    }
}

/* Restart one of the threads that are waiting on the condition variable */
int
SDL_CondSignal(SDL_cond * cond)
{
    if (!cond) {
        return SDL_SetError("Passed a NULL condition variable");
    }

    /* If there are waiting threads not already signalled, then
       signal the condition and wait for the thread to respond.
     */
    while (1) {
        int waiting = cond->waiting;
        int signals = cond->signals;
        if (waiting <= signals) {
            break;
        }
        if (cond_cas(&cond->signals, signals, signals + 1)) {
            semaphore_quit(cond->wait_sem);
            semaphore_enter(cond->wait_done);
            break;
        }
    }

    return 0;
}

/* Restart all threads that are waiting on the condition variable */
int
SDL_CondBroadcast(SDL_cond * cond)
{
    if (!cond) {
        return SDL_SetError("Passed a NULL condition variable");
    }

    /* If there are waiting threads not already signalled, then
       signal the condition and wait for the thread to respond.
     */
    while (1) {
        int waiting = cond->waiting;
        int signals = cond->signals;
        int num_waiting;
        int i;

        if (waiting <= signals) {
            break;
        }

        num_waiting = (waiting - signals);
        if (!cond_cas(&cond->signals, signals, waiting)) {
            continue;
        }

        for (i = 0; i < num_waiting; ++i) {
            semaphore_quit(cond->wait_sem);
        }
        /* Now all released threads are blocked here, waiting for us.
           Collect them all (and win fabulous prizes!) :-)
         */
        for (i = 0; i < num_waiting; ++i) {
            semaphore_enter(cond->wait_done);
        }
        break;
    }

    return 0;
}

/* Wait on the condition variable for at most 'ms' milliseconds.
   The mutex must be locked before entering this function!
   The mutex is unlocked during the wait, and locked again after the wait.

Typical use:

Thread A:
    SDL_LockMutex(lock);
    while ( ! condition ) {
        SDL_CondWait(cond, lock);
    }
    SDL_UnlockMutex(lock);

Thread B:
    SDL_LockMutex(lock);
    ...
    condition = true;
    ...
    SDL_CondSignal(cond);
    SDL_UnlockMutex(lock);
 */
int
SDL_CondWaitTimeout(SDL_cond * cond, SDL_mutex * mutex, Uint32 ms)
{
    int retval;

    if (!cond) {
        return SDL_SetError("Passed a NULL condition variable");
    }

    /* Increment the number of waiters. This allows the signal mechanism
       to only perform a signal if there are waiting threads.
     */
    __sync_fetch_and_add(&cond->waiting, 1);

    /* Unlock the mutex, as is required by condition variable semantics */
    SDL_UnlockMutex(mutex);

    /* Wait for a signal */
    if (ms == SDL_MUTEX_MAXWAIT) {
        semaphore_enter(cond->wait_sem);
        retval = 0;
    } else {
        /* the kernel semaphore has no timed wait; poll with a short sleep */
        Uint32 start = SDL_GetTicks();
        retval = SDL_MUTEX_TIMEDOUT;
        while ((SDL_GetTicks() - start) < ms) {
            if (semaphore_tryenter(cond->wait_sem) == 0) {
                retval = 0;
                break;
            }
            SDL_Delay(1);
        }
    }

    /* Let the signaler know we have completed the wait, otherwise
       the signaler can race ahead and get the condition semaphore
       if we are stopped between the mutex unlock and semaphore wait,
       giving a deadlock.  See the following URL for details:
       http://web.archive.org/web/20010914175514/http://www-classic.be.com/aboutbe/benewsletter/volume_III/Issue40.html#Workshop
     */
    while (1) {
        int signals = cond->signals;
        if (signals <= 0) {
            break;
        }
        if (!cond_cas(&cond->signals, signals, signals - 1)) {
            continue;
        }
        /* If we timed out, we need to eat a condition signal */
        if (retval > 0) {
            semaphore_enter(cond->wait_sem);
        }
        /* We always notify the signal thread that we are done */
        semaphore_quit(cond->wait_done);
        break;
    }
    __sync_fetch_and_add(&cond->waiting, -1);

    /* Lock the mutex, as is required by condition variable semantics */
    SDL_LockMutex(mutex);

    return retval;
}

/* Wait on the condition variable forever */
int
SDL_CondWait(SDL_cond * cond, SDL_mutex * mutex)
{
    return SDL_CondWaitTimeout(cond, mutex, SDL_MUTEX_MAXWAIT);
}

/* vi: set ts=4 sw=4 expandtab: */
