#ifndef SHM_H
#define SHM_H

#include <shmflag.h>

#ifdef __cplusplus
extern "C" {
#endif

void* shm_alloc(unsigned int size, int flag);
void* shm_map(void* p);
int   shm_unmap(void* p);

#ifdef __cplusplus
}
#endif

#endif
