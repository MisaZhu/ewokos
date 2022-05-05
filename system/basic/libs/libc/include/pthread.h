#ifndef PTHREAD_H
#define PTHREAD_H

#include <sys/ewokdef.h>

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

int pthread_create(pthread_t* thread,
		const pthread_attr_t *attr,
		void *(*start_routine) (void *),
		void *arg);

pthread_t pthread_self(void);

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t *attr);

int pthread_mutex_lock(pthread_mutex_t* mutex);

int pthread_mutex_unlock(pthread_mutex_t* mutex);

#ifdef __cplusplus
}
#endif

#endif
