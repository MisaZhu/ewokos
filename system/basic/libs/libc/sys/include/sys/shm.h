#ifndef SHM_H
#define SHM_H

#ifdef __cplusplus
extern "C" {
#endif


enum {
	SHM_FAMILY = 0,
	SHM_PUBLIC
};

void* shm_alloc(unsigned int size, int flag);
void* shm_map(void* p);
int   shm_unmap(void* p);

#ifdef __cplusplus
}
#endif

#endif
