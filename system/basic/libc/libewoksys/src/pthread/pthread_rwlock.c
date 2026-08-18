#include <pthread.h>
#include <ewoksys/syscall.h>
#include <string.h>
#include <errno.h>

#define RWLOCK_MAGIC 0x52574c31u /* "RWL1" */

static volatile int _rw_init_locked = 0;

static void rw_init_lock(void) {
	while(__sync_lock_test_and_set(&_rw_init_locked, 1) != 0)
		syscall0(SYS_YIELD);
}

static void rw_init_unlock(void) {
	__sync_lock_release(&_rw_init_locked);
}

static int rwlock_do_init(pthread_rwlock_t *rwlock) {
	memset(&rwlock->lock, 0, sizeof(rwlock->lock));
	if(pthread_cond_init(&rwlock->rcond, NULL) != 0)
		return ENOMEM;
	if(pthread_cond_init(&rwlock->wcond, NULL) != 0) {
		pthread_cond_destroy(&rwlock->rcond);
		return ENOMEM;
	}
	rwlock->readers = 0;
	rwlock->writer = 0;
	rwlock->wr_waiters = 0;
	rwlock->magic = RWLOCK_MAGIC;
	return 0;
}

/* Support PTHREAD_RWLOCK_INITIALIZER (all-zero) via lazy init. */
static int rwlock_ensure(pthread_rwlock_t *rwlock) {
	if(rwlock->magic == RWLOCK_MAGIC)
		return 0;

	rw_init_lock();
	int res = 0;
	if(rwlock->magic != RWLOCK_MAGIC)
		res = rwlock_do_init(rwlock);
	rw_init_unlock();
	return res;
}

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr) {
	(void)attr;
	if(rwlock == NULL)
		return EINVAL;
	return rwlock_do_init(rwlock);
}

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock) {
	if(rwlock == NULL || rwlock->magic != RWLOCK_MAGIC)
		return EINVAL;

	pthread_cond_destroy(&rwlock->rcond);
	pthread_cond_destroy(&rwlock->wcond);
	if(rwlock->lock != 0)
		pthread_mutex_destroy(&rwlock->lock);
	rwlock->magic = 0;
	return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) {
	int res = rwlock_ensure(rwlock);
	if(res != 0)
		return res;

	pthread_mutex_lock(&rwlock->lock);
	while(rwlock->writer || rwlock->wr_waiters > 0)
		pthread_cond_wait(&rwlock->rcond, &rwlock->lock);
	rwlock->readers++;
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock) {
	int res = rwlock_ensure(rwlock);
	if(res != 0)
		return res;

	pthread_mutex_lock(&rwlock->lock);
	if(rwlock->writer || rwlock->wr_waiters > 0) {
		pthread_mutex_unlock(&rwlock->lock);
		return EBUSY;
	}
	rwlock->readers++;
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, const struct timespec *abstime) {
	int res = rwlock_ensure(rwlock);
	if(res != 0)
		return res;
	if(abstime == NULL)
		return EINVAL;

	pthread_mutex_lock(&rwlock->lock);
	while(rwlock->writer || rwlock->wr_waiters > 0) {
		res = pthread_cond_timedwait(&rwlock->rcond, &rwlock->lock, abstime);
		if(res == ETIMEDOUT) {
			pthread_mutex_unlock(&rwlock->lock);
			return ETIMEDOUT;
		}
	}
	rwlock->readers++;
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
	int res = rwlock_ensure(rwlock);
	if(res != 0)
		return res;

	pthread_mutex_lock(&rwlock->lock);
	rwlock->wr_waiters++;
	while(rwlock->writer || rwlock->readers > 0)
		pthread_cond_wait(&rwlock->wcond, &rwlock->lock);
	rwlock->wr_waiters--;
	rwlock->writer = 1;
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) {
	int res = rwlock_ensure(rwlock);
	if(res != 0)
		return res;

	pthread_mutex_lock(&rwlock->lock);
	if(rwlock->writer || rwlock->readers > 0) {
		pthread_mutex_unlock(&rwlock->lock);
		return EBUSY;
	}
	rwlock->writer = 1;
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock, const struct timespec *abstime) {
	int res = rwlock_ensure(rwlock);
	if(res != 0)
		return res;
	if(abstime == NULL)
		return EINVAL;

	pthread_mutex_lock(&rwlock->lock);
	rwlock->wr_waiters++;
	while(rwlock->writer || rwlock->readers > 0) {
		res = pthread_cond_timedwait(&rwlock->wcond, &rwlock->lock, abstime);
		if(res == ETIMEDOUT) {
			rwlock->wr_waiters--;
			pthread_mutex_unlock(&rwlock->lock);
			return ETIMEDOUT;
		}
	}
	rwlock->wr_waiters--;
	rwlock->writer = 1;
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
	if(rwlock == NULL || rwlock->magic != RWLOCK_MAGIC)
		return EINVAL;

	pthread_mutex_lock(&rwlock->lock);
	if(rwlock->writer)
		rwlock->writer = 0;
	else if(rwlock->readers > 0)
		rwlock->readers--;

	if(!rwlock->writer && rwlock->readers == 0) {
		if(rwlock->wr_waiters > 0)
			pthread_cond_signal(&rwlock->wcond);
		else
			pthread_cond_broadcast(&rwlock->rcond);
	}
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}
