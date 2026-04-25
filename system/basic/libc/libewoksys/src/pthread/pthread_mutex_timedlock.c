#include <pthread.h>
#include <ewoksys/thread.h>
#include <ewoksys/semaphore.h>
#include <ewoksys/syscall.h>
#include <ewoksys/sys.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

// Get current time in microseconds
static inline uint64_t get_time_usec(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Try to acquire mutex lock (non-blocking)
int pthread_mutex_trylock(pthread_mutex_t* mutex) {
	if(mutex == NULL)
		return EINVAL;

	if(*mutex == 0) {
		*mutex = semaphore_alloc();
		if(*mutex == 0)
			return ENOMEM;
	}

	// Try to enter semaphore, non-blocking
	int res = semaphore_tryenter(*mutex);
	if(res == -2) {
		// Semaphore is occupied
		return EBUSY;
	}
	return (res == 0) ? 0 : EINVAL;
}

// Acquire mutex lock with timeout
int pthread_mutex_timedlock(pthread_mutex_t* mutex, const struct timespec *abstime) {
	if(mutex == NULL || abstime == NULL)
		return EINVAL;

	if(*mutex == 0) {
		*mutex = semaphore_alloc();
		if(*mutex == 0)
			return ENOMEM;
	}

	// Calculate timeout time point in microseconds
	uint64_t timeout_usec = (uint64_t)abstime->tv_sec * 1000000ULL + 
	                        (abstime->tv_nsec / 1000);
	uint64_t start_time = get_time_usec();

	while(1) {
		// Try to acquire semaphore
		int res = semaphore_tryenter(*mutex);
		
		if(res == 0) {
			// Successfully acquired
			return 0;
		}
		
		if(res != -2) {
			// Other errors
			return EINVAL;
		}

		// Check if timeout
		uint64_t current_time = get_time_usec();
		if(current_time >= timeout_usec) {
			// Timeout
			return ETIMEDOUT;
		}

		// Short sleep before retry (use yield to reduce CPU usage)
		uint64_t remaining = timeout_usec - current_time;
		if(remaining > 1000) {
			// More than 1ms remaining, sleep 1ms
			usleep(1000);
		} else if(remaining > 100) {
			// Less than 1ms but more than 100us, sleep half of remaining time
			usleep(remaining / 2);
		} else {
			// About to timeout, just yield
			syscall0(SYS_YIELD);
		}
	}
}
