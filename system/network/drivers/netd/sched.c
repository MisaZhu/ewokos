#include <pthread.h>
#include <time.h>
#include <sys/errno.h>
#include <unistd.h>
#include <sys/time.h>

#include <ewoksys/proc.h>

#include "platform.h"
#include "stack/util.h"

/* Memory barrier for ARM64 to ensure cache coherency */
#define memory_barrier() __sync_synchronize()

int
sched_ctx_init(struct sched_ctx *ctx)
{
    ctx->available = 1;
	ctx->cond = 0;
	ctx->interrupted = 0;
	ctx->wc = 0;
	ctx->sleeping = 0;
    return 0;
}

int
sched_ctx_destroy(struct sched_ctx *ctx)
{
	return 0;
}

int
sched_sleep(struct sched_ctx *ctx, mutex_t *mutex, const struct timeval *abstime)
{
    int ret = 1;
    struct timeval now;
    int loop_count = 0;
    
    if (ctx->interrupted) {
        errorf("[SCHED_SLEEP] ctx=%p, already interrupted, return -1", ctx);
        errno = EINTR;
        return -1;
    }
    
    /*
     * CRITICAL FIX: Use a blocking flag to synchronize with sched_wakeup.
     * This prevents the race condition where:
     * 1. Thread A checks ctx->cond (false), releases mutex
     * 2. Thread B sets ctx->cond = 1, calls proc_wakeup (but A hasn't blocked yet)
     * 3. Thread A calls proc_block_by and misses the wakeup
     * 
     * Solution: Set a "blocking" flag BEFORE releasing the mutex.
     * sched_wakeup will always call proc_wakeup regardless of wc count,
     * so even if we miss the signal, the next sched_wakeup will wake us.
     * We also add a short sleep to reduce CPU usage in race scenarios.
     */
    
    // Mark that we are about to block
    ctx->wc++;
    /* Memory barrier to ensure wc is visible to other cores before we release mutex */
    memory_barrier();

    // Check condition while still holding the mutex
    memory_barrier();
    if (ctx->cond || !ctx->available) {
        ctx->wc--;
        memory_barrier();
        return 0;
    }
    mutex_unlock(mutex);

    while (1) {
        loop_count++;
        
        // Check condition before blocking
        memory_barrier();
        if (ctx->cond || !ctx->available) {
            ret = 0;
            break;
        }

        // Check for timeout if abstime is provided
        if (abstime) {
            uint64_t usec;
            kernel_tic(NULL, &usec);
            now.tv_sec = usec / 1000000;
            now.tv_usec = usec % 1000000;
            if (timercmp(&now, abstime, >)) {
                errorf("[SCHED_SLEEP] ctx=%p, loop=%d, TIMEOUT, break", ctx, loop_count);
                errno = ETIMEDOUT;
                ret = -1;
                break;
            }
        }

        // Limit loop count to prevent infinite spinning in case of bugs
        if (loop_count > 100000) {
            errorf("[SCHED_SLEEP] ctx=%p, too many loops (%d), possible bug", ctx, loop_count);
            errno = EAGAIN;
            ret = -1;
            break;
        }
        
        // Mark that we are about to sleep
        ctx->sleeping = 1;
        memory_barrier();
        
        /* CRITICAL: Re-check cond before actually blocking.
         * This prevents the race where sched_wakeup was called between
         * checking cond and setting sleeping=1. If cond is already set,
         * we skip blocking entirely to avoid missing the wakeup. */
        memory_barrier();
        if (!ctx->cond && ctx->available) {
            proc_block_by(getpid(), (uint32_t)ctx);
        }
        
        // Mark that we are awake
        ctx->sleeping = 0;
        memory_barrier();
        
        // CRITICAL: Immediately check condition after waking up.
        // This prevents the race where a spurious wakeup occurs but cond is still 0.
        memory_barrier();
        if (ctx->cond || !ctx->available) {
            ret = 0;
            break;
        }
    }

    mutex_lock(mutex);

    // Clear the condition after waking up to prevent spurious wakeups
    ctx->cond = 0;

    ctx->wc--;
    /* Memory barrier to ensure wc update is visible */
    memory_barrier();
    
    if (ctx->interrupted) {
        if (!ctx->wc) {
            ctx->interrupted = 0;
        }
        errno = EINTR;
        return -1;
    }
    return ret;
}

int
sched_wakeup(struct sched_ctx *ctx)
{
    ctx->cond = 1;
    /* Memory barrier to ensure cond is written before proc_wakeup */
    memory_barrier();
    
    /* Wait for sched_sleep to actually set sleeping flag.
     * This ensures we don't call proc_wakeup before sched_sleep has blocked.
     * We only wait briefly (100us max) - if sched_sleep hasn't blocked yet,
     * the cond flag will cause it to skip blocking entirely. */
    int wait_count = 0;
    while (ctx->wc > 0 && !ctx->sleeping && wait_count < 10) {
        proc_usleep(10);
        wait_count++;
    }
    
    proc_wakeup((uint32_t)ctx);
    return 0;
}

int
sched_interrupt(struct sched_ctx *ctx)
{
    ctx->interrupted = 1;
    ctx->cond = 1;
    /* Memory barrier to ensure cond is written before proc_wakeup */
    memory_barrier();
    // Wake up any process blocked on this ctx
    proc_wakeup((uint32_t)ctx);
    return 0;
}
