#ifndef LOCKC_H
#define LOCKC_H

#include <stdint.h>

enum {
	LOCK_CMD_NEW = 0,
	LOCK_CMD_FREE,
	LOCK_CMD_LOCK,
	LOCK_CMD_UNLOCK
};

uint32_t lock_new(void);
int lock_free(uint32_t lock);
int lock_lock(uint32_t lock);
int lock_unlock(uint32_t lock);

#endif
