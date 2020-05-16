#ifndef LOCKC_H
#define LOCKC_H

#include <stdint.h>

uint32_t lock_new(void);
int lock_free(uint32_t lock);
int lock_lock(uint32_t lock);
int lock_unlock(uint32_t lock);

#endif
