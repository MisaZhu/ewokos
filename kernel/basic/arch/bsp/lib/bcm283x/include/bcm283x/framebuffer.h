#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <dev/fbinfo.h>

int32_t   bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep);
fbinfo_t* bcm283x_get_fbinfo(void); 

#endif