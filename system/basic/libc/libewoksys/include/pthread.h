#ifndef PTHREAD_H
#define PTHREAD_H

#include <ewoksys/ewokdef.h>
#include <stddef.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PTHREAD_KEYS_MAX 64
#define PTHREAD_DESTRUCTOR_ITERATIONS 4

#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

#define PTHREAD_ONCE_INIT 0

#define PTHREAD_MUTEX_INITIALIZER 0
#define PTHREAD_RWLOCK_INITIALIZER { 0 }
#define PTHREAD_BARRIER_SERIAL_THREAD (-1)

/* cancellation constants (cancellation itself is not supported) */
#define PTHREAD_CANCEL_ENABLE  0
#define PTHREAD_CANCEL_DISABLE 1
#define PTHREAD_CANCEL_DEFERRED     0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1

typedef int32_t pthread_t;
typedef struct { 
	uint32_t attr;
	uint32_t detachstate;
	size_t stacksize;
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

typedef int pthread_once_t;
typedef int pthread_key_t;

typedef struct {
	uint32_t magic;        // validity marker
	pthread_mutex_t lock;  // protects state below
	int readers;           // active reader count
	int writer;            // writer present flag
	int wr_waiters;        // queued writer count
	pthread_cond_t rcond;  // reader wake-up
	pthread_cond_t wcond;  // writer wake-up
} pthread_rwlock_t;

typedef struct {
	uint32_t attr;
} pthread_rwlockattr_t;

typedef struct {
	uint32_t magic;        // validity marker
	pthread_mutex_t lock;
	pthread_cond_t cond;
	unsigned threshold;    // required arrival count
	unsigned gen;          // barrier generation
	unsigned arrived;      // arrivals in current generation
} pthread_barrier_t;

typedef struct {
	uint32_t attr;
} pthread_barrierattr_t;

// Thread operations
int pthread_create(pthread_t* thread,
		const pthread_attr_t *attr,
		void *(*start_routine) (void *),
		void *arg);

pthread_t pthread_self(void);

int pthread_equal(pthread_t t1, pthread_t t2);

int pthread_join(pthread_t thread, void **retval);

int pthread_detach(pthread_t thread);

void pthread_exit(void *retval);

int pthread_cancel(pthread_t thread);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void);

// Thread attribute operations
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);

// Mutex operations
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_timedlock(pthread_mutex_t* mutex, const struct timespec *abstime);
int pthread_mutex_unlock(pthread_mutex_t* mutex);
int pthread_mutex_destroy(pthread_mutex_t* mutex);

// Mutex attribute operations
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);

// Condition variable operations
int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t* cond);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);

// Condition variable attribute operations
int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);

// Once-init and thread-specific data
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
void *pthread_getspecific(pthread_key_t key);

// Read-write locks
int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, const struct timespec *abstime);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock, const struct timespec *abstime);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

// Barriers
int pthread_barrier_init(pthread_barrier_t *barrier,
		const pthread_barrierattr_t *attr, unsigned count);
int pthread_barrier_destroy(pthread_barrier_t *barrier);
int pthread_barrier_wait(pthread_barrier_t *barrier);

/* internal: release TLS of the current thread (called from pthread_exit
 * and from the pthread_create trampoline) */
void __pthread_tls_thread_exit(void);

#ifdef __cplusplus
}
#endif

#endif
