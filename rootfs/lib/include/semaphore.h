#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <types.h>

typedef int32_t  semaphore_t;

semaphore_t semaphore_alloc();
void semaphore_free(semaphore_t s);
int32_t semaphore_lock(semaphore_t s);
int32_t semaphore_unlock(semaphore_t s);


#endif
