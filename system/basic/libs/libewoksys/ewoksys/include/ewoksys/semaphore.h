#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifdef __cplusplus
extern "C" {
#endif

int  semaphore_alloc(void);
void semaphore_free(int sem_id);
int  semaphore_enter(int sem_id);
int  semaphore_quit(int sem_id);

#ifdef __cplusplus
}
#endif


#endif