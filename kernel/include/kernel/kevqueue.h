#ifndef KEVENT_QUEUE_H
#define KEVENT_QUEUE_H

#include <kevent.h>
#include <proto.h>

void       kev_init(void);
kevent_t*  kev_push(uint32_t type, const proto_t* data);
kevent_t*  kev_pop(void);

#endif
