#ifndef EWOKOS_SEMAPHORE_H
#define EWOKOS_SEMAPHORE_H

#include <stdint.h>
#include <sys/types.h>

struct timespec;

#ifdef __cplusplus
extern "C" {
#endif

#define SEM_VALUE_MAX 0x7fffffff
#define SEM_FAILED    ((sem_t *)-1)

#define SEM_MAGIC 0x53454d31u /* "SEM1" */

/*
 * A sem_t is a handle on a kernel counting semaphore: the permits live in the
 * kernel, so sem_wait() blocks there and sem_post() wakes a waiter directly.
 *
 * It used to carry the count in userspace with a kernel binary semaphore as a
 * spinlock around it, which meant sem_wait() on an empty semaphore could only
 * poll - try to take the count, fail, SYS_YIELD, repeat - and burned a core per
 * waiter. There is no userspace count left to protect.
 */
typedef struct sem_t {
	int32_t ksem;      /* kernel counting semaphore holding the permits */
	uint32_t magic;
} sem_t;

int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_destroy(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);

/* Named semaphores are not supported. */
sem_t *sem_open(const char *name, int oflag, ...);
int sem_close(sem_t *sem);
int sem_unlink(const char *name);

#ifdef __cplusplus
}
#endif

#endif
