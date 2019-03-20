#ifndef SHAREMEM_H
#define SHAREMEM_H

#include <types.h>

int32_t shm_alloc(uint32_t size);

void shm_free(int32_t id);

void* shm_map(int32_t id);

int32_t shm_unmap(int32_t id);

#endif
