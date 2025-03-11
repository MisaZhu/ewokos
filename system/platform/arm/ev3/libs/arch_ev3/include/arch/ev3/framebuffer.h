#ifndef EV3_FRAMEBUFFER_H
#define EV3_FRAMEBUFFER_H

#include <ewoksys/fbinfo.h>

fbinfo_t* ev3_get_fbinfo(void);
int32_t ev3_fb_init(uint32_t w, uint32_t h, uint32_t dep);

#endif
