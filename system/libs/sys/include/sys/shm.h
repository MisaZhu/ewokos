#ifndef SHM_H
#define SHM_H

enum {
	SHM_FAMILY = 0,
	SHM_PUBLIC
};

int   shm_alloc(unsigned int size, int flag);
void* shm_map(int shmid);
int   shm_unmap(int shmid);

#endif
