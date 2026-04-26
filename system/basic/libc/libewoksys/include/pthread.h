#ifndef PTHREAD_H
#define PTHREAD_H

#include <ewoksys/ewokdef.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef int32_t pthread_t;
typedef struct { 
	uint32_t attr;
} pthread_attr_t;

typedef int32_t pthread_mutex_t;
typedef struct {
	uint32_t attr;
} pthread_mutexattr_t;

// Condition variable definition
typedef struct {
	int32_t sem_wait;      // Waiting semaphore
	int32_t sem_signal;    // Signal notification semaphore
	volatile int waiters;  // Waiter count
	volatile int signaled; // Signal flag
} pthread_cond_t;

typedef struct {
	uint32_t attr;
} pthread_condattr_t;

// Thread operations
int pthread_create(pthread_t* thread,
		const pthread_attr_t *attr,
		void *(*start_routine) (void *),
		void *arg);

pthread_t pthread_self(void);

int pthread_join(pthread_t thread, void **retval);

int pthread_detach(pthread_t thread);

// Mutex operations
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_timedlock(pthread_mutex_t* mutex, const struct timespec *abstime);
int pthread_mutex_unlock(pthread_mutex_t* mutex);
int pthread_mutex_destroy(pthread_mutex_t* mutex);

// Condition variable operations
int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t* cond);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);

#ifdef __cplusplus
}
#endif

#endif
