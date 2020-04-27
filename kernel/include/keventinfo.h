#ifndef KEVENTINFO_H
#define KEVENTINFO_H

#include <_types.h>
#include <proto.h>

enum {
	KEV_NONE = 0,
	KEV_FCLOSED,
	KEV_PROC_EXIT,
	KEV_GLOBAL_SET,
	KEV_US_INT
};

typedef struct {
	uint32_t type;	
	proto_t* data;
} kevent_t;

#endif
