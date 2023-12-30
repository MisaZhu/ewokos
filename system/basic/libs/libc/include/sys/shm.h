#ifndef SYS_SHM_H
#define SYS_SHM_H

#include <sys/ipc.h>

int shmget(key_t key, int size, int flag);
int *shmat(int shmid, const void *addr, int flag);
int shmdt(void *p);

#endif