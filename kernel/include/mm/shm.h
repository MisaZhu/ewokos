#ifndef SHAREMEM_H
#define SHAREMEM_H

#include <types.h>

void shm_init(void);

int32_t shm_alloc(uint32_t size);

void* shm_raw(int32_t id);

void shm_free(int32_t id);

void* shm_proc_map(int32_t pid, int32_t id);

int32_t shm_proc_unmap(int32_t pid, int32_t id);

void shm_proc_free(int32_t pid);

#endif
