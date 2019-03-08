#ifndef SHAREMEM_H
#define SHAREMEM_H

#include <types.h>

int32_t shmalloc(uint32_t size);

void shmfree(int32_t id);

void* shmMap(int32_t id);

int32_t shmUnmap(int32_t id);

#endif
