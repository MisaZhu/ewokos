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
	void* data;
} kevent_t;

#endif
