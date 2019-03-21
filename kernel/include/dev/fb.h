#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <fbinfo.h>

extern char _fb_start[];

bool fb_init();

fb_info_t* fb_get_info();

#endif
