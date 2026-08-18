#include <pthread.h>
#include <string.h>
#include <stddef.h>

/* Mutex/condition attribute objects only carry hints; all attributes are
 * accepted and ignored because the kernel semaphore has a fixed policy. */

#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_ERRORCHECK 2

int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
	if(attr == NULL)
		return -1;
	memset(attr, 0, sizeof(pthread_mutexattr_t));
	return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) {
	if(attr == NULL)
		return -1;
	memset(attr, 0, sizeof(pthread_mutexattr_t));
	return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
	if(attr == NULL ||
			(type != PTHREAD_MUTEX_NORMAL &&
			 type != PTHREAD_MUTEX_RECURSIVE &&
			 type != PTHREAD_MUTEX_ERRORCHECK))
		return -1;
	attr->attr = (uint32_t)type;
	return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type) {
	if(attr == NULL || type == NULL)
		return -1;
	*type = (int)attr->attr;
	return 0;
}

int pthread_condattr_init(pthread_condattr_t *attr) {
	if(attr == NULL)
		return -1;
	memset(attr, 0, sizeof(pthread_condattr_t));
	return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *attr) {
	if(attr == NULL)
		return -1;
	memset(attr, 0, sizeof(pthread_condattr_t));
	return 0;
}
