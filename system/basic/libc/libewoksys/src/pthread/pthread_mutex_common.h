#ifndef EWOKOS_PTHREAD_MUTEX_COMMON_H
#define EWOKOS_PTHREAD_MUTEX_COMMON_H

#include <pthread.h>
#include <ewoksys/semaphore.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>

/*
 * Futex-style mutex, packed into the single int32_t that pthread_mutex_t has
 * always been.
 *
 * The old implementation made a mutex BE a kernel semaphore: every lock was a
 * SYS_SEMAPHORE_ENTER and every unlock a SYS_SEMAPHORE_QUIT, even with zero
 * contention, and each of those syscalls also takes the kernel's global proc
 * lock. svcinfo showed semaphore_enter/semaphore_quit dominating the syscall
 * mix purely because mutexes are everywhere (netd's per-packet stack lock,
 * vfsd's async worker, g2dd, every pthread app).
 *
 * The word now carries two things:
 *   bits [0:1]  state   0 = FREE, 1 = HELD (no waiters), 2 = HELD (waiters)
 *   bits [2:..] sem id  kernel park channel, 0 until first contention
 *
 * SEMAPHORE_MAX is 1024, so an id shifted left by two never reaches bit 13 and
 * the whole word stays a small positive int - which is why pthread_mutex_t can
 * keep its exact size and its "0 == valid, unlocked" property. Nothing outside
 * this file changes: PTHREAD_MUTEX_INITIALIZER (0), the `= 0` static
 * initializers, netd's uint32_t mutex_t punned through (void*), and
 * pthread_rwlock's `if(lock != 0)` destroy guard all stay correct.
 *
 * Uncontended lock/unlock is now a single CAS on the word with NO syscall. The
 * kernel semaphore is allocated lazily on the first contention and used only as
 * a park/wake channel (a counting semaphore that starts empty), so blocking
 * stays FIFO and precise per mutex - no shared-bucket spurious/lost wakes.
 *
 * Ownership lives entirely in the state bits; a permit from the semaphore is
 * only a "someone unlocked, re-check the word" hint. That is what makes a wake
 * that races an acquire harmless: the woken task loops and CASes the word
 * again. Every transition is a whole-word CAS, so a thread that reads
 * state == 2 always reads the sem id that was published before it (a waiter
 * runs mtx_ensure_sem() before it announces itself), and an unlock that races a
 * new waiter's publish fails its CAS and retries against the fresh word.
 */

#define MTX_STATE_MASK  0x3
#define MTX_FREE        0x0
#define MTX_HELD        0x1
#define MTX_WAIT        0x2   /* held, with at least one waiter registered */
#define MTX_SEM_SHIFT   2

static inline int32_t mtx_load(int32_t* w) {
    return *(volatile int32_t*)w;
}

static inline int mtx_state(int32_t w) {
    return (int)(w & MTX_STATE_MASK);
}

static inline int mtx_sem(int32_t w) {
    return (int)((uint32_t)w >> MTX_SEM_SHIFT);
}

/* CAS the state bits to @st, preserving the sem id; whole-word compare. */
static inline int mtx_cas_state(int32_t* w, int32_t expect, int st) {
    int32_t nw = (expect & ~MTX_STATE_MASK) | (int32_t)st;
    return __sync_bool_compare_and_swap(w, expect, nw);
}

/* Monotonic-enough wall clock for the timed path (same source it always used). */
static inline uint64_t mtx_now_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

/*
 * Allocate the park channel on first contention and publish it into the word,
 * preserving whatever state bits are live. Exactly one racer wins the publish;
 * the loser frees the id it allocated (same proc, so the free always lands).
 * Returns the sem id (> 0), or 0 if the kernel is out of semaphores.
 */
static inline int mtx_ensure_sem(int32_t* w) {
    int32_t cur = mtx_load(w);
    int id = mtx_sem(cur);
    if(id != 0)
        return id;

    id = semaphore_alloc_count(0);   /* empty counting sem: park until posted */
    if(id == 0)
        return 0;

    while(1) {
        cur = mtx_load(w);
        if(mtx_sem(cur) != 0) {      /* someone published first */
            semaphore_free(id);
            return mtx_sem(cur);
        }
        int32_t nw = (cur & MTX_STATE_MASK) | (int32_t)(id << MTX_SEM_SHIFT);
        if(__sync_bool_compare_and_swap(w, cur, nw))
            return id;
        /* state changed under us; re-read and retry the publish */
    }
}

/* Fast path: take the lock only if it is completely free. No syscall. */
static inline int mtx_try_acquire(int32_t* w) {
    int32_t cur = mtx_load(w);
    if(mtx_state(cur) != MTX_FREE)
        return 0;
    return mtx_cas_state(w, cur, MTX_HELD);
}

/*
 * Contended acquisition. Loops: grab the word if it goes free, otherwise
 * register as a waiter and park on the channel until an unlock posts a hint.
 *
 * A task that acquires through here marks the word WAIT (not HELD): it arrived
 * via contention, so there may be fellow waiters its own unlock must wake. If
 * it turns out to be alone, that unlock posts one stray permit, which the next
 * park consumes - harmless and self-balancing.
 *
 * @timed selects the deadline path; @abstime is a wall-clock instant as for
 * pthread_cond_timedwait. On give-up it posts one wake so a permit that was
 * handed to it (kernel grants to the oldest waiter) is not lost with it, which
 * would otherwise strand the next waiter behind a free lock.
 */
static inline int mtx_lock_contended(int32_t* w, const struct timespec* abstime, int timed) {
    uint64_t deadline = 0;
    if(timed)
        deadline = (uint64_t)abstime->tv_sec * 1000000ULL +
                   (uint64_t)(abstime->tv_nsec / 1000);

    while(1) {
        int32_t cur = mtx_load(w);
        if(mtx_state(cur) == MTX_FREE) {
            if(mtx_cas_state(w, cur, MTX_WAIT))
                return 0;
            continue;
        }

        uint64_t rem = 0;
        if(timed) {
            uint64_t now = mtx_now_usec();
            if(now >= deadline) {
                int sem = mtx_sem(cur);
                if(sem != 0)
                    semaphore_quit(sem);   /* don't strand a fellow waiter */
                return ETIMEDOUT;
            }
            rem = deadline - now;
            if(rem > 0xffffffffULL)
                rem = 0xffffffffULL;
        }

        /* Publish the channel BEFORE announcing ourselves, so any unlock that
         * observes WAIT also observes a valid sem id to post to. */
        int sem = mtx_ensure_sem(w);
        if(sem == 0)
            return ENOMEM;

        /*
         * Park ONLY once the word truly reads WAIT.
         *
         * The holder can release in the window while we allocated the channel,
         * and a free lock's unlock takes the HELD branch and posts NOTHING - so
         * parking on a FREE word sleeps forever on a lock nobody holds. That
         * lost wakeup stranded vfsd's async worker and hung the whole system.
         * Announce HELD->WAIT ourselves, or observe a WAIT a fellow waiter set;
         * if the word reads FREE, loop back and take it instead of parking.
         * WAIT always implies a live holder (the only WAIT->FREE transition is
         * that holder's unlock, which posts in the same step), so an announced
         * park always has a wake coming - possibly via a chain of handoffs.
         */
        int announced = 0;
        while(1) {
            cur = mtx_load(w);
            int st = mtx_state(cur);
            if(st == MTX_FREE)
                break;                              /* re-contend for ownership */
            if(st == MTX_WAIT) {
                announced = 1;                      /* a fellow waiter flagged it */
                break;
            }
            if(mtx_cas_state(w, cur, MTX_WAIT)) {   /* HELD -> WAIT */
                announced = 1;
                break;
            }
            /* CAS lost: the word changed under us; re-read and decide again */
        }
        if(!announced)
            continue;

        int r = timed ? semaphore_enter_timeout(sem, (uint32_t)rem)
                      : semaphore_enter(sem);
        if(r == SEM_RES_ERROR)
            return EINVAL;   /* destroyed under us */
        /* ACQUIRED (a wake hint) or a TIMEOUT slice: loop and re-contend. The
         * deadline check at the top is what finally settles a timed give-up. */
    }
}

#endif
