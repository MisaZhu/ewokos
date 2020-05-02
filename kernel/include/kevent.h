#ifndef KEVENT_H
#define KEVENT_H

#include <_types.h>
#include <proto.h>

enum {
	KEV_NONE = 0,
	KEV_FCLOSED,
	KEV_PROC_EXIT,
	KEV_PROC_CREATED,
	KEV_US_INT,
	KEV_GLOBAL_SET
};

typedef struct {
	uint32_t type;	
	proto_t* data;
} kevent_t;

#endif
