#ifndef SHAREMEM_H
#define SHAREMEM_H

#include <types.h>

void shmInit();

int32_t shmalloc(uint32_t size);

void shmfree(int32_t id);

void* shmProcMap(int32_t id);

int32_t shmProcUnmap(int32_t id);

void shmProcFree(int32_t pid);

#endif
