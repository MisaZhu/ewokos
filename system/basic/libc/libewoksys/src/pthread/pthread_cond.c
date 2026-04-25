#include <pthread.h>
#include <ewoksys/semaphore.h>
#include <ewoksys/syscall.h>
#include <ewoksys/sys.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// Get current time in microseconds
static inline uint64_t get_time_usec(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Convert timespec to microseconds
static inline uint64_t timespec_to_usec(const struct timespec *ts) {
	if(ts == NULL) return 0;
	return (uint64_t)ts->tv_sec * 1000000ULL + (ts->tv_nsec / 1000);
}

// Initialize condition variable
int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t *attr) {
	(void)attr;
	if(cond == NULL)
		return EINVAL;
	
	memset(cond, 0, sizeof(pthread_cond_t));
	
	// Allocate waiting semaphore
	cond->sem_wait = semaphore_alloc();
	if(cond->sem_wait == 0)
		return ENOMEM;
	
	// Allocate signal notification semaphore
	cond->sem_signal = semaphore_alloc();
	if(cond->sem_signal == 0) {
		semaphore_free(cond->sem_wait);
		cond->sem_wait = 0;
		return ENOMEM;
	}
	
	cond->waiters = 0;
	cond->signaled = 0;
	
	return 0;
}

// Destroy condition variable
int pthread_cond_destroy(pthread_cond_t* cond) {
	if(cond == NULL)
		return EINVAL;
	
	if(cond->sem_wait != 0) {
		semaphore_free(cond->sem_wait);
		cond->sem_wait = 0;
	}
	
	if(cond->sem_signal != 0) {
		semaphore_free(cond->sem_signal);
		cond->sem_signal = 0;
	}
	
	cond->waiters = 0;
	cond->signaled = 0;
	
	return 0;
}

// Internal implementation for condition variable wait
static int cond_wait_internal(pthread_cond_t* cond, pthread_mutex_t* mutex, 
                               const struct timespec *abstime, int timed) {
	if(cond == NULL || mutex == NULL)
		return EINVAL;
	
	if(cond->sem_wait == 0 || cond->sem_signal == 0)
		return EINVAL;
	
	// Increment waiter count
	__atomic_fetch_add(&cond->waiters, 1, __ATOMIC_SEQ_CST);
	
	// Release mutex
	int unlock_res = pthread_mutex_unlock(mutex);
	if(unlock_res != 0) {
		__atomic_fetch_sub(&cond->waiters, 1, __ATOMIC_SEQ_CST);
		return unlock_res;
	}
	
	int result = 0;
	
	if(timed && abstime != NULL) {
		// Timed wait
		uint64_t timeout_usec = timespec_to_usec(abstime);
		uint64_t start_time = get_time_usec();
		
		while(1) {
			// Check if signaled
			int signaled = __atomic_load_n(&cond->signaled, __ATOMIC_SEQ_CST);
			if(signaled > 0) {
				// Consume one signal
				if(__atomic_compare_exchange_n(&cond->signaled, &signaled, signaled - 1,
				                               false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
					result = 0;
					break;
				}
				continue;
			}
			
			// Try to acquire waiting semaphore
			int res = semaphore_tryenter(cond->sem_wait);
			if(res == 0) {
				result = 0;
				break;
			}
			
			// Check timeout
			uint64_t current_time = get_time_usec();
			if(current_time >= timeout_usec) {
				result = ETIMEDOUT;
				break;
			}
			
			// Short sleep
			uint64_t remaining = timeout_usec - current_time;
			if(remaining > 1000) {
				usleep(1000);
			} else if(remaining > 100) {
				usleep(remaining / 2);
			} else {
				syscall0(SYS_YIELD);
			}
		}
	} else {
		// Infinite wait
		while(1) {
			// Check if signaled
			int signaled = __atomic_load_n(&cond->signaled, __ATOMIC_SEQ_CST);
			if(signaled > 0) {
				// Consume one signal
				if(__atomic_compare_exchange_n(&cond->signaled, &signaled, signaled - 1,
				                               false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
					result = 0;
					break;
				}
				continue;
			}
			
			// Try to acquire waiting semaphore
			int res = semaphore_tryenter(cond->sem_wait);
			if(res == 0) {
				result = 0;
				break;
			}
			
			// Short yield
			syscall0(SYS_YIELD);
		}
	}
	
	// Decrement waiter count
	__atomic_fetch_sub(&cond->waiters, 1, __ATOMIC_SEQ_CST);
	
	// Re-acquire mutex
	int lock_res = pthread_mutex_lock(mutex);
	if(lock_res != 0 && result == 0) {
		result = lock_res;
	}
	
	return result;
}

// Wait on condition variable
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
	return cond_wait_internal(cond, mutex, NULL, 0);
}

// Wait on condition variable with timeout
int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, 
                            const struct timespec *abstime) {
	if(abstime == NULL)
		return EINVAL;
	return cond_wait_internal(cond, mutex, abstime, 1);
}

// Signal one waiting thread
int pthread_cond_signal(pthread_cond_t* cond) {
	if(cond == NULL)
		return EINVAL;
	
	if(cond->sem_wait == 0 || cond->sem_signal == 0)
		return EINVAL;
	
	// Increment signal count
	__atomic_fetch_add(&cond->signaled, 1, __ATOMIC_SEQ_CST);
	
	// If there are waiters, release waiting semaphore
	int waiters = __atomic_load_n(&cond->waiters, __ATOMIC_SEQ_CST);
	if(waiters > 0) {
		semaphore_quit(cond->sem_wait);
	}
	
	return 0;
}

// Broadcast to all waiting threads
int pthread_cond_broadcast(pthread_cond_t* cond) {
	if(cond == NULL)
		return EINVAL;
	
	if(cond->sem_wait == 0 || cond->sem_signal == 0)
		return EINVAL;
	
	// Get current waiter count
	int waiters = __atomic_load_n(&cond->waiters, __ATOMIC_SEQ_CST);
	if(waiters > 0) {
		// Set signal count to waiter count
		__atomic_store_n(&cond->signaled, waiters, __ATOMIC_SEQ_CST);
		
		// Release all waiting semaphores
		for(int i = 0; i < waiters; i++) {
			semaphore_quit(cond->sem_wait);
		}
	}
	
	return 0;
}
