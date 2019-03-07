#ifndef SHAREMEM_H
#define SHAREMEM_H

#include <types.h>

void shmInit();

void* shmalloc(uint32_t size);

void shmfree(void* p);

int32_t shmProcMap(void* p);

int32_t shmProcUnmap(void* p);

#endif
