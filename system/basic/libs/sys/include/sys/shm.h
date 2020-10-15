#ifndef SHM_H
#define SHM_H

#ifdef __cplusplus
extern "C" {
#endif


enum {
	SHM_FAMILY = 0,
	SHM_PUBLIC
};

int   shm_alloc(unsigned int size, int flag);
void* shm_map(int shmid);
int   shm_unmap(int shmid);

#ifdef __cplusplus
}
#endif

#endif
