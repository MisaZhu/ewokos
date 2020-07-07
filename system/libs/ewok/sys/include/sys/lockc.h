#ifndef LOCKC_H
#define LOCKC_H

#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t lock_t;

lock_t lock_new(void);
void lock_free(lock_t lock);
int lock_lock(lock_t lock);
void lock_unlock(lock_t lock);

#ifdef __cplusplus
}
#endif

#endif
