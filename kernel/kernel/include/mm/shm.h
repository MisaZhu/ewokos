#ifndef SHAREMEM_H
#define SHAREMEM_H

#include <stdint.h>

void    shm_init(void);
int32_t shm_get(int32_t key, uint32_t size, int32_t flag);
uint32_t shm_alloced_size(void);
//void*   shm_raw(int32_t id);

void*   shm_proc_map(int32_t pid, int32_t id);
int32_t shm_proc_unmap(int32_t pid, void *p);

#endif
