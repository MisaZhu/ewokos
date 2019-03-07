#ifndef SHAREMEM_H
#define SHAREMEM_H

#include <types.h>

void* shmalloc(uint32_t size);

int32_t shmMap(void* p);

int32_t shmUnmap(void* p);

#endif
