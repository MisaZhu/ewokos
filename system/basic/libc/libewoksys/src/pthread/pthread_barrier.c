#include <pthread.h>
#include <ewoksys/syscall.h>
#include <string.h>
#include <errno.h>

#define BARRIER_MAGIC 0x42415231u /* "BAR1" */

static volatile int _bar_init_locked = 0;

static void bar_init_lock(void) {
	while(__sync_lock_test_and_set(&_bar_init_locked, 1) != 0)
		syscall0(SYS_YIELD);
}

static void bar_init_unlock(void) {
	__sync_lock_release(&_bar_init_locked);
}

static int barrier_do_init(pthread_barrier_t *barrier, unsigned count) {
	memset(&barrier->lock, 0, sizeof(barrier->lock));
	if(pthread_cond_init(&barrier->cond, NULL) != 0)
		return ENOMEM;
	barrier->threshold = count;
	barrier->gen = 0;
	barrier->arrived = 0;
	barrier->magic = BARRIER_MAGIC;
	return 0;
}

int pthread_barrier_init(pthread_barrier_t *barrier,
		const pthread_barrierattr_t *attr, unsigned count) {
	(void)attr;
	if(barrier == NULL || count == 0)
		return EINVAL;
	return barrier_do_init(barrier, count);
}

int pthread_barrier_destroy(pthread_barrier_t *barrier) {
	if(barrier == NULL || barrier->magic != BARRIER_MAGIC)
		return EINVAL;

	pthread_cond_destroy(&barrier->cond);
	if(barrier->lock != 0)
		pthread_mutex_destroy(&barrier->lock);
	barrier->magic = 0;
	return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier) {
	if(barrier == NULL)
		return EINVAL;

	if(barrier->magic != BARRIER_MAGIC) {
		bar_init_lock();
		int res = 0;
		if(barrier->magic != BARRIER_MAGIC)
			res = barrier_do_init(barrier, 1);
		bar_init_unlock();
		if(res != 0)
			return res;
	}

	pthread_mutex_lock(&barrier->lock);
	unsigned gen = barrier->gen;
	barrier->arrived++;
	if(barrier->arrived >= barrier->threshold) {
		barrier->arrived = 0;
		barrier->gen++;
		pthread_cond_broadcast(&barrier->cond);
		pthread_mutex_unlock(&barrier->lock);
		return PTHREAD_BARRIER_SERIAL_THREAD;
	}
	while(barrier->gen == gen)
		pthread_cond_wait(&barrier->cond, &barrier->lock);
	pthread_mutex_unlock(&barrier->lock);
	return 0;
}
