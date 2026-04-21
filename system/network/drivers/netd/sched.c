#include <pthread.h>
#include <time.h>
#include <sys/errno.h>
#include <unistd.h>
#include <sys/time.h>

#include <ewoksys/proc.h>

#include "platform.h"

int
sched_ctx_init(struct sched_ctx *ctx)
{
    ctx->available = 1;
	ctx->cond = 0;
	ctx->interrupted = 0;
	ctx->wc = 0;
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
    if (ctx->interrupted) {
        errno = EINTR;
        return -1;
    }
    ctx->wc++;

    mutex_unlock(mutex);

    while (1) {
        // Check condition before blocking to avoid race condition
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
                errno = ETIMEDOUT;
                ret = -1;
                break;
            }
        }

        // Use real blocking instead of polling
        // Block on this sched_ctx, waiting for sched_wakeup to signal
        proc_block_by(getpid(), (uint32_t)ctx);
        // After waking up, immediately check condition again
        // This handles the case where wakeup arrived just after blocking
    }

    mutex_lock(mutex);

    // Clear the condition after waking up to prevent spurious wakeups
    ctx->cond = 0;

    ctx->wc--;
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
    // Wake up any process blocked on this ctx
    proc_wakeup((uint32_t)ctx);
    return 0;
}

int
sched_interrupt(struct sched_ctx *ctx)
{
    ctx->interrupted = 1;
    ctx->cond = 1;
    // Wake up any process blocked on this ctx
    proc_wakeup((uint32_t)ctx);
    return 0;
}
