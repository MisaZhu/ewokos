#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>
#include <kernel/context.h>

/*
 * A kernel semaphore is a counting permit pool with a FIFO wait queue.
 *
 * count      permits available right now. A task takes one to enter and
 *            returns one on quit.
 * max_count  the ceiling. quit() refuses to push count past it, which is what
 *            keeps an unmatched quit() on a binary mutex (max_count == 1) from
 *            silently inflating the permit pool and letting two holders in.
 *
 * Two shapes are handed out:
 *   semaphore_alloc()        count = 1, max_count = 1.  A binary mutex. This is
 *                            what pthread_mutex_t is, and it is the only shape
 *                            that tracks occupied_pid, so it is also the only
 *                            one semaphore_clear_occupied() can recover when a
 *                            thread dies holding it.
 *   semaphore_alloc_count(n) count = n, max_count unbounded. A POSIX counting
 *                            semaphore (sem_t). Ownership is meaningless there
 *                            - sem_post() is legal from a task that never waited.
 */
typedef struct {
	int32_t creater_pid;
	int32_t occupied_pid;
	int8_t  occupied;
	int32_t count;
	int32_t max_count;
} semaphore_t;

enum {
	SEM_IDLE = 0,
	SEM_OCCUPIED
};

/*
 * Uncapped ceiling for a counting semaphore; matches SEM_VALUE_MAX in libc's
 * <semaphore.h>.
 */
#define SEMAPHORE_COUNT_MAX 0x7fffffff

/*
 * Results of an enter, delivered to the caller in ctx->gpr[0].
 *
 * ACQUIRED  a permit is ours.
 * ERROR     no such semaphore, or it was freed / its owner died while we
 *           waited. The caller must not retry.
 * OCCUPIED  the kernel could not park us, so nothing was waited on: either the
 *           wait queue is full, or this context is synchronously serving an ipc
 *           request and must never block (see proc_ipc_sync_serving). The
 *           caller yields and retries - a narrow fallback, not the normal path.
 * RETRY     we did park and were released without a permit: a spurious generic
 *           wake, or proc_block_by() refused to sleep on a latched edge. The
 *           caller re-issues the enter immediately - the kernel keeps its place
 *           in the wait queue across the round trip, so this costs FIFO order
 *           nothing. Distinct from OCCUPIED only so libc can skip the yield.
 * TIMEOUT   the budget passed to semaphore_enter_timeout() was consumed without
 *           a permit. Only ever returned by the timed entry point; the absolute
 *           deadline belongs to the caller, so it recomputes the remaining
 *           budget and may retry.
 */
#define SEM_RES_ACQUIRED   0
#define SEM_RES_ERROR     (-1)
#define SEM_RES_OCCUPIED  (-2)
#define SEM_RES_TIMEOUT   (-3)
#define SEM_RES_RETRY     (-4)

void semaphore_init(void);

int32_t semaphore_alloc(void);
int32_t semaphore_alloc_count(int32_t count);
void    semaphore_free(uint32_t sem_id);
void    semaphore_enter(context_t* ctx, uint32_t sem_id);
void    semaphore_enter_timeout(context_t* ctx, uint32_t sem_id, uint32_t timeout_usec);
void    semaphore_try_enter(context_t* ctx, uint32_t sem_id);
int32_t semaphore_get_count(uint32_t sem_id);
int32_t semaphore_quit(uint32_t sem_id);
void    semaphore_clear(int32_t pid);
void    semaphore_clear_occupied(int32_t pid);

#endif
