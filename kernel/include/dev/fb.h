#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <fbinfo.h>

extern char _fb_start[];

bool _fb_init();

fb_info_t* _fb_get_info();

#endif
