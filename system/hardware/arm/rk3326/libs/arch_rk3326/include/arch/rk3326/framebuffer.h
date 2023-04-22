#ifndef RK3326_FRAMEBUFFER_H
#define RK3326_FRAMEBUFFER_H

#include <sys/fbinfo.h>

fbinfo_t* rk3326_get_fbinfo(void);
int32_t rk3326_fb_init(uint32_t w, uint32_t h, uint32_t dep);

#endif
