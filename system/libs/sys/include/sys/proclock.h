#ifndef PROC_LOCK_H
#define PROC_LOCK_H

#include <stdint.h>

typedef uint32_t proc_lock_t;

proc_lock_t proc_lock_new(void);
void proc_lock_free(proc_lock_t lock);
void proc_lock(proc_lock_t lock);
void proc_unlock(proc_lock_t lock);

#endif
