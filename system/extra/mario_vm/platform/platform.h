#ifndef MARIO_PLATFORM_H
#define MARIO_PLATFORM_H

extern void* (*_platform_malloc)(uint32_t size);
extern void  (*_platform_free)(void* p);

void platform_init(void);

#endif