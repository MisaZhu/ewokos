#ifndef KEVENT_DEVICE_H
#define KEVENT_DEVICE_H

#include <types.h>
#include <kevent.h>

extern bool kevent_init(void);
extern void dev_kevent_push(uint32_t ev);
extern int32_t dev_kevent_read(int16_t id, void* buf, uint32_t size);

#endif
