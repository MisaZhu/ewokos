#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <types.h>

void semaphore_init();
int32_t semaphore_alloc();
void semaphore_free(int32_t s);

int32_t semaphore_lock(int32_t s);
int32_t semaphore_unlock(int32_t s);

#endif
