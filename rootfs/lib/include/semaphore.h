#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <types.h>

typedef int32_t  semaphore_t;

int32_t semaphore_init(semaphore_t* s);
int32_t semaphore_close(semaphore_t* s);
int32_t semaphore_lock(semaphore_t* s);
int32_t semaphore_unlock(semaphore_t* s);


#endif
