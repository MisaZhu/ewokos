#ifndef SHAREMEM_H
#define SHAREMEM_H

#include <stdint.h>
#include <kernel/proc.h>

void    shm_init(void);
int32_t shm_get(int32_t key, uint32_t size, int32_t flag);
void*   shm_map(proc_t* proc, int32_t key, uint32_t size, int32_t flag, int32_t* id);
uint32_t shm_alloced_size(void);
//void*   shm_raw(int32_t id);

void*   shm_proc_map(proc_t* proc, int32_t id);
int32_t shm_proc_unmap(proc_t* proc, void *p);
int32_t shm_proc_unmap_by_id(proc_t* proc, uint32_t id, bool free_it);
int32_t shm_set_owner(uint32_t id, int32_t pid);

#endif
