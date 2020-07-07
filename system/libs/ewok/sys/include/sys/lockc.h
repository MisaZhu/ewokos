#ifndef LOCKC_H
#define LOCKC_H

#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif


uint32_t lock_new(void);
void lock_free(uint32_t lock);
int lock_lock(uint32_t lock);
void lock_unlock(uint32_t lock);

#ifdef __cplusplus
}
#endif

#endif
