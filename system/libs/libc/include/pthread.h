#ifndef PTHREAD_H
#define PTHREAD_H

#include <stdint.h>

typedef int32_t pthread_t;

typedef struct { 
	uint32_t attr;
} pthread_attr_t;

void pthread_create(pthread_t* thread, const pthread_attr_t *attr,
		void *(*start_routine) (void *), void *arg);

#endif
