#ifndef FB_H
#define FB_H

#include <dev/fbinfo.h>

int32_t fb_init(fbinfo_t* fbinfo);
void    fb_flush32(uint32_t* g32, uint32_t w, uint32_t h);

#endif