#ifndef SYS_SHM_H
#define SYS_SHM_H

#include <sys/ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

int shmget(key_t key, int size, int flag);
void *shmat(int shmid, const void *addr, int flag);
int shmdt(void *p);


#ifdef __cplusplus
}
#endif

#endif