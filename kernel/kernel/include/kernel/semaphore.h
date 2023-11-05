#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>
#include <kernel/context.h>

typedef struct {
	int32_t creater_pid;
	int32_t occupied_pid;
	int8_t  occupied;
} semaphore_t;

enum {
	SEM_IDLE = 0,
	SEM_OCCUPIED
};

void semaphore_init(void);

int32_t semaphore_alloc(void);
void    semaphore_free(uint32_t sem_id);
int32_t semaphore_enter(context_t* ctx, uint32_t sem_id);
int32_t semaphore_quit(uint32_t sem_id);
void    semaphore_clear(int32_t pid);

#endif