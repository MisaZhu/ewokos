#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Results of the enter calls. These mirror SEM_RES_* in the kernel's
 * kernel/semaphore.h, repeated here because the kernel headers are not part of
 * the userspace ABI.
 *
 * ACQUIRED  the permit is ours.
 * ERROR     no such semaphore, or it was freed while we waited. Do not retry.
 * OCCUPIED  semaphore_tryenter() only: a permit was not free. From the blocking
 *           calls it means the kernel could not park us and the caller is
 *           retrying internally, so it never reaches userspace.
 * TIMEOUT   semaphore_enter_timeout() only: the budget ran out.
 * RETRY     internal to semaphore_enter(): the kernel parked us and released us
 *           without a permit (a spurious generic wake). Retried in place, so it
 *           never reaches userspace either.
 */
#define SEM_RES_ACQUIRED   0
#define SEM_RES_ERROR     (-1)
#define SEM_RES_OCCUPIED  (-2)
#define SEM_RES_TIMEOUT   (-3)
#define SEM_RES_RETRY     (-4)

/*
 * semaphore_alloc() hands out a binary semaphore: one permit, and quit()
 * refuses to add a second. That cap is what makes pthread_mutex_unlock() on an
 * unlocked mutex fail instead of quietly letting two holders in.
 *
 * semaphore_alloc_count() hands out a counting semaphore with `count` permits
 * available immediately and no ceiling, so a task that never waited may post
 * (POSIX sem_post, pthread_cond_broadcast). Ownership is not tracked for these.
 */
int  semaphore_alloc(void);
int  semaphore_alloc_count(int count);
void semaphore_free(int sem_id);

/*
 * Blocking. Returns once a permit is ours (0) or the semaphore is gone (-1).
 * The kernel queues the caller and hands the permit over directly to the oldest
 * waiter on release, so waiting costs no cpu.
 */
int  semaphore_enter(int sem_id);

/*
 * Blocking with a relative budget in microseconds; 0 means wait forever, as it
 * does in the kernel. The budget is measured against the kernel tic, which is
 * monotonic, so a wall-clock adjustment cannot stretch or cut short the wait.
 * Callers holding a POSIX absolute deadline convert it to a remaining budget
 * and pass that.
 *
 * On expiry it makes one last non-blocking attempt before reporting TIMEOUT, so
 * a permit granted to us in the gap between the kernel giving up and us looking
 * is collected rather than stranded.
 */
int  semaphore_enter_timeout(int sem_id, uint32_t timeout_usec);

/*
 * Non-blocking: 0 if a permit was taken, SEM_RES_OCCUPIED if none was free.
 *
 * Also settles any wait-queue entry this task left behind on a timed park, so
 * it is safe - and necessary - to call once after giving up on a deadline.
 */
int  semaphore_tryenter(int sem_id);

/* Permits available now, or -1 if there is no such semaphore. */
int  semaphore_get_count(int sem_id);

/* Return a permit, waking the oldest waiter. 0 on success, -1 on refusal. */
int  semaphore_quit(int sem_id);

#ifdef __cplusplus
}
#endif


#endif
