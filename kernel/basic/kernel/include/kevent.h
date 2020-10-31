#ifndef KEVENT_H
#define KEVENT_H

#include <stdint.h>

enum {
	KEV_NONE = 0,
	KEV_FCLOSED,
	KEV_PROC_EXIT,
	KEV_PROC_CREATED,
	KEV_US_INT
};

typedef struct {
	uint32_t type;	
	uint32_t data[3];
} kevent_t;

#endif
