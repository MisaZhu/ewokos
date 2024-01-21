#ifndef KEVENT_QUEUE_H
#define KEVENT_QUEUE_H

#include <kevent.h>

void       kev_init(void);
kevent_t*  kev_push(uint32_t type, uint32_t arg0, uint32_t arg1, uint32_t arg2);
int32_t    kev_pop(kevent_t* kev);

#endif
