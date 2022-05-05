#ifndef FB_H
#define FB_H

#include <dev/fbinfo.h>

int32_t fb_init(uint32_t w, uint32_t h, fbinfo_t* fbinfo);

#endif