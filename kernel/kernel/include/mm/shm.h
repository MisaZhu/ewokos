#ifndef SHAREMEM_H
#define SHAREMEM_H

#include <stdint.h>

void    shm_init(void);
void*   shm_alloc(uint32_t size, int32_t flag);
int32_t shm_alloced_size(void);
//void*   shm_raw(int32_t id);

void*   shm_proc_map(int32_t pid, void* p);
int32_t shm_proc_unmap(int32_t pid, void* p);
int32_t shm_proc_ref(int32_t pid, void* p);

#endif
