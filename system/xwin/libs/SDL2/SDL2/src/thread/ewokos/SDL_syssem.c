/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software; however, the code below was
  adapted for EwokOS.
*/
#include "../../SDL_internal.h"

/* An implementation of semaphores using the EwokOS kernel semaphore
   primitives directly.

   EwokOS note: the original implementation layered SDL_mutex/SDL_cond on
   top of SDL_sem while SDL_CreateMutex() itself creates an SDL_sem, which
   recursed infinitely (CreateSemaphore -> CreateCond -> CreateMutex ->
   CreateSemaphore) and silently blew the stack.  Using the kernel
   primitives here breaks that cycle. */

#include "SDL_timer.h"
#include "SDL_thread.h"
#include "SDL_systhread_c.h"

#include <ewoksys/semaphore.h>


struct SDL_semaphore
{
    int sem_id;
};

#if SDL_THREADS_DISABLED

SDL_sem *
SDL_CreateSemaphore(Uint32 initial_value)
{
    SDL_sem* sem = (SDL_sem *) SDL_malloc(sizeof(*sem));
    if (!sem) {
        SDL_OutOfMemory();
        return NULL;
    }
    sem->sem_id = semaphore_alloc_count((int)initial_value);
    if (sem->sem_id <= 0) {
        SDL_free(sem);
        return NULL;
    }
    return sem;
}

void
SDL_DestroySemaphore(SDL_sem * sem)
{
    if (sem != NULL) {
        semaphore_free(sem->sem_id);
        SDL_free(sem);
    }
}

int
SDL_SemTryWait(SDL_sem * sem)
{
    if (!sem) {
        return SDL_SetError("Passed a NULL semaphore");
    }
    return (semaphore_tryenter(sem->sem_id) == 0) ? 0 : SDL_MUTEX_TIMEDOUT;
}

int
SDL_SemWaitTimeout(SDL_sem * sem, Uint32 timeout)
{
    if (!sem) {
        return SDL_SetError("Passed a NULL semaphore");
    }
    if (timeout == 0) {
        return SDL_SemTryWait(sem);
    }
    semaphore_enter(sem->sem_id);
    return 0;
}

int
SDL_SemWait(SDL_sem * sem)
{
    return SDL_SemWaitTimeout(sem, SDL_MUTEX_MAXWAIT);
}

Uint32
SDL_SemValue(SDL_sem * sem)
{
    return 0;
}

int
SDL_SemPost(SDL_sem * sem)
{
    if (!sem) {
        return SDL_SetError("Passed a NULL semaphore");
    }
    semaphore_quit(sem->sem_id);
    return 0;
}

#else


SDL_sem *
SDL_CreateSemaphore(Uint32 initial_value)
{
    SDL_sem *sem;

    sem = (SDL_sem *) SDL_malloc(sizeof(*sem));
    if (!sem) {
        SDL_OutOfMemory();
        return NULL;
    }

    /* semaphore_alloc() is a binary mutex that starts with its single permit
       free, so SDL_CreateSemaphore(0) on it returned an immediately
       acquirable semaphore: SDL_CreateThread's SDL_SemWait(args->wait) fell
       through, args was freed, and the new thread read garbage in
       SDL_RunThread. SDL semaphores are counting and start at initial_value,
       which is what semaphore_alloc_count() provides. */
    sem->sem_id = semaphore_alloc_count((int)initial_value);
    if (sem->sem_id <= 0) {
        SDL_free(sem);
        return NULL;
    }

    return sem;
}

/* WARNING:
   You cannot call this function when another thread is using the semaphore.
*/
void
SDL_DestroySemaphore(SDL_sem * sem)
{
    if (sem) {
        semaphore_free(sem->sem_id);
        SDL_free(sem);
    }
}

int
SDL_SemTryWait(SDL_sem * sem)
{
    if (!sem) {
        return SDL_SetError("Passed a NULL semaphore");
    }
    return (semaphore_tryenter(sem->sem_id) == 0) ? 0 : SDL_MUTEX_TIMEDOUT;
}

int
SDL_SemWaitTimeout(SDL_sem * sem, Uint32 timeout)
{
    Uint32 start;

    if (!sem) {
        return SDL_SetError("Passed a NULL semaphore");
    }

    /* A timeout of 0 is an easy case */
    if (timeout == 0) {
        return SDL_SemTryWait(sem);
    }

    if (timeout == SDL_MUTEX_MAXWAIT) {
        semaphore_enter(sem->sem_id);
        return 0;
    }

    /* The kernel semaphore has no timed wait; poll with a short sleep. */
    start = SDL_GetTicks();
    while (semaphore_tryenter(sem->sem_id) != 0) {
        if ((SDL_GetTicks() - start) >= timeout) {
            return SDL_MUTEX_TIMEDOUT;
        }
        SDL_Delay(1);
    }
    return 0;
}

int
SDL_SemWait(SDL_sem * sem)
{
    return SDL_SemWaitTimeout(sem, SDL_MUTEX_MAXWAIT);
}

Uint32
SDL_SemValue(SDL_sem * sem)
{
    /* the kernel semaphore exposes no value query */
    return 0;
}

int
SDL_SemPost(SDL_sem * sem)
{
    if (!sem) {
        return SDL_SetError("Passed a NULL semaphore");
    }
    semaphore_quit(sem->sem_id);
    return 0;
}

#endif /* SDL_THREADS_DISABLED */
/* vi: set ts=4 sw=4 expandtab: */
