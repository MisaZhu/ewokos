#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <types.h>

int32_t semaphore_init(int32_t* s);
int32_t semaphore_close(int32_t* s);

int32_t semaphore_lock(int32_t* s);
int32_t semaphore_unlock(int32_t* s);

#endif
