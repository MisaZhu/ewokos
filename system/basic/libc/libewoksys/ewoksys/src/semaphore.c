#include <ewoksys/semaphore.h>
#include <ewoksys/syscall.h>
#include <ewoksys/kernel_tic.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int  semaphore_alloc(void) {
    return (int)syscall0(SYS_SEMAPHORE_ALLOC);
}

int  semaphore_alloc_count(int count) {
    return (int)syscall1(SYS_SEMAPHORE_ALLOC_COUNT, (ewokos_addr_t)count);
}

void semaphore_free(int sem_id) {
    syscall1(SYS_SEMAPHORE_FREE, (ewokos_addr_t)sem_id);
}

/*
 * Blocking acquire.
 *
 * The kernel parks us on the semaphore's wait queue and hands the permit over
 * directly when it is released, so this returns only once the permit is ours
 * (0) or the semaphore is gone (-1).
 *
 * SEM_RES_RETRY means the kernel did park us but released us without a permit:
 * a generic token-0 wake (an ipc return, a signal) that was not meant for us,
 * or proc_block_by() declining to sleep on an edge already latched for this
 * task. Our place in the queue survives the round trip, so re-issue at once and
 * the kernel re-parks us in the same FIFO position. Both causes are consumed by
 * the trip, which is what keeps this from spinning.
 *
 * SEM_RES_OCCUPIED means the kernel declined to park us at all: either its wait
 * queue is full, or this context is synchronously serving an ipc request, which
 * must never block or it strands the server's saved-state restore machine. Both
 * persist across an instant, so yield before retrying. This is a narrow
 * fallback - before the kernel grew a blocking wait queue it was the ONLY path,
 * and every contended mutex or condition variable in the system burned a core
 * spinning here.
 */
int  semaphore_enter(int sem_id) {
    while(true) {
        int res = (int)syscall1(SYS_SEMAPHORE_ENTER, (ewokos_addr_t)sem_id);
        if(res == SEM_RES_ACQUIRED || res == SEM_RES_ERROR)
            return res;
        if(res != SEM_RES_OCCUPIED)
            continue;   /* SEM_RES_RETRY: re-park in place, no yield */
        syscall0(SYS_YIELD);
    }
}

/*
 * Monotonic and, via the vsyscall page, free of a syscall. The wall clock is
 * only a fallback for the brief window before that page is mapped; a caller
 * establishes its deadline with one call and measures against the same source
 * afterwards, so the two are never mixed within one wait.
 */
static uint64_t sem_now_usec(void) {
    uint64_t usec = 0;
    if(kernel_tic(NULL, &usec) == 0)
        return usec;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

int  semaphore_enter_timeout(int sem_id, uint32_t timeout_usec) {
    if(timeout_usec == 0)
        return semaphore_enter(sem_id);

    uint64_t deadline = sem_now_usec() + (uint64_t)timeout_usec;

    while(true) {
        uint64_t now = sem_now_usec();
        if(now >= deadline)
            break;

        uint64_t rem = deadline - now;
        uint32_t budget = (rem > 0xffffffffULL) ? 0xffffffffu : (uint32_t)rem;

        int res = (int)syscall2(SYS_SEMAPHORE_ENTER_TIMEOUT,
                (ewokos_addr_t)sem_id, (ewokos_addr_t)budget);
        if(res == SEM_RES_ACQUIRED || res == SEM_RES_ERROR)
            return res;
        if(res == SEM_RES_OCCUPIED)
            syscall0(SYS_YIELD);
        /*
         * SEM_RES_TIMEOUT from a single kernel attempt covers both "the slice
         * elapsed" and "an unrelated wake released the block" - the kernel
         * cannot tell them apart and does not need to. Recomputing what is left
         * of the deadline here settles it, which also keeps the absolute
         * deadline out of the kernel: POSIX hands callers a wall-clock abstime
         * and the kernel only has a boot-relative tic.
         */
    }

    /*
     * Giving up. A timed park stages its verdict before it blocks, so the last
     * attempt left our wait-queue entry in the kernel and a permit may have been
     * handed to it after we stopped looking. One tryenter settles both: it
     * collects that grant, so the permit is never stranded with a task that has
     * walked away, and drops the stale entry so a later release cannot hand a
     * permit to a task that is no longer waiting.
     */
    int res = semaphore_tryenter(sem_id);
    if(res == SEM_RES_ACQUIRED || res == SEM_RES_ERROR)
        return res;
    return SEM_RES_TIMEOUT;
}

int  semaphore_tryenter(int sem_id) {
    return (int)syscall1(SYS_SEMAPHORE_TRY_ENTER, (ewokos_addr_t)sem_id);
}

int  semaphore_get_count(int sem_id) {
    return (int)syscall1(SYS_SEMAPHORE_GET_COUNT, (ewokos_addr_t)sem_id);
}

int  semaphore_quit(int sem_id) {
    return (int)syscall1(SYS_SEMAPHORE_QUIT, (ewokos_addr_t)sem_id);
}

#ifdef __cplusplus
}
#endif
