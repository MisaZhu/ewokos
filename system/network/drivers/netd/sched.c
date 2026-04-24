#include <pthread.h>
#include <time.h>
#include <sys/errno.h>
#include <unistd.h>
#include <sys/time.h>

#include <ewoksys/proc.h>

#include "platform.h"
#include "stack/util.h"

#define memory_barrier() __sync_synchronize()

#define SCHED_POLL_INTERVAL_US 1000 /* 1ms */

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
    memory_barrier();

    if (ctx->cond || !ctx->available) {
        ctx->wc--;
        memory_barrier();
        return 0;
    }
    pthread_mutex_unlock((pthread_mutex_t*)mutex);

    while (1) {
        memory_barrier();
        if (ctx->cond || !ctx->available) {
            ret = 0;
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

    pthread_mutex_lock((pthread_mutex_t*)mutex);

    ctx->cond = 0;

    ctx->wc--;
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
