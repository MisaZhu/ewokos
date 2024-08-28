#include <pthread.h>
#include <time.h>
#include <sys/errno.h>
#include <unistd.h>

#include "platform.h"

int
sched_ctx_init(struct sched_ctx *ctx)
{
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
sched_sleep(struct sched_ctx *ctx, mutex_t *mutex, const struct timespec *abstime)
{
    int ret;
	(void)abstime;
    if (ctx->interrupted) {
        errno = EINTR;
        return -1;
    }
    ctx->wc++;
	ctx->cond = 0;
	
	mutex_unlock(mutex);
    while(1){
		if(ctx->cond){
			ret = 0;
			break;
		}
		proc_usleep(10000);
    }
	mutex_lock(mutex);

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
	return 0;
}

int
sched_interrupt(struct sched_ctx *ctx)
{
	ctx->interrupted = 1;
	ctx->cond = 1;
	return 0;
}
