#include <time.h>
#include <sys/errno.h>
#include <unistd.h>
#include <sys/time.h>

#include <ewoksys/proc.h>

#include "platform.h"
#include "stack/util.h"

#define SCHED_POLL_INTERVAL_US 1000 /* 1ms */

#ifdef __aarch64__
    #define memory_barrier() __asm__ __volatile__("isb" ::: "memory")
#elif defined(__arm__) && (__ARM_ARCH > 6)
    #define memory_barrier() __asm__ __volatile__("isb" ::: "memory")
#else
    #define memory_barrier() __asm__ __volatile__("" ::: "memory")
#endif

int
sched_ctx_init(struct sched_ctx *ctx)
{
    ctx->available = 1;
    ctx->cond = 0;
    ctx->interrupted = 0;
    ctx->wc = 0;
    ctx->sleeping = 0;
    ctx->wakeups = 0;
    ctx->destroyed = 0;
    return 0;
}

int
sched_ctx_destroy(struct sched_ctx *ctx)
{
    /* Just set flags, do NOT wait. The waiters will exit on their own. */
    ctx->destroyed = 1;
    ctx->cond = 1;
    memory_barrier();
    return 0;
}

int
sched_sleep(struct sched_ctx *ctx, mutex_t *mutex, const struct timeval *abstime)
{
    int ret = 1;
    struct timeval now;

    if (ctx->destroyed) {
        errno = EINTR;
        return -1;
    }

    ctx->wc++;
    memory_barrier();

    if (ctx->cond || !ctx->available || ctx->destroyed) {
        ctx->wc--;
        memory_barrier();
        return 0;
    }

    ctx->sleeping = 1;
    memory_barrier();

    mutex_unlock(mutex);

    while (1) {
        memory_barrier();
        if (ctx->cond || !ctx->available || ctx->destroyed) {
            break;
        }

        if (ctx->interrupted) {
            errno = EINTR;
            ret = -1;
            break;
        }

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

        usleep(SCHED_POLL_INTERVAL_US);
    }

    mutex_lock(mutex);

    ctx->cond = 0;
    ctx->wakeups = 0;

    ctx->wc--;
    ctx->sleeping = 0;
    memory_barrier();

    if (ctx->destroyed) {
        errno = EINTR;
        return -1;
    }

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
    memory_barrier();
    return 0;
}

int
sched_interrupt(struct sched_ctx *ctx)
{
    ctx->interrupted = 1;
    ctx->cond = 1;
    memory_barrier();
    return 0;
}
