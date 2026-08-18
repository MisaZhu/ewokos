#include <pthread.h>
#include <string.h>
#include <stddef.h>

int pthread_attr_init(pthread_attr_t *attr) {
	if(attr == NULL)
		return -1;
	memset(attr, 0, sizeof(pthread_attr_t));
	attr->detachstate = PTHREAD_CREATE_JOINABLE;
	return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
	if(attr == NULL)
		return -1;
	memset(attr, 0, sizeof(pthread_attr_t));
	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
	if(attr == NULL ||
			(detachstate != PTHREAD_CREATE_JOINABLE &&
			 detachstate != PTHREAD_CREATE_DETACHED))
		return -1;
	attr->detachstate = (uint32_t)detachstate;
	return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
	if(attr == NULL || detachstate == NULL)
		return -1;
	*detachstate = (int)attr->detachstate;
	return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
	if(attr == NULL)
		return -1;
	attr->stacksize = stacksize;
	return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize) {
	if(attr == NULL || stacksize == NULL)
		return -1;
	*stacksize = attr->stacksize;
	return 0;
}
