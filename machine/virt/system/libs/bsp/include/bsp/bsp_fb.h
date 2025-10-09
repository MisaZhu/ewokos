#ifndef BSP_FRAMEBUFFER_H
#define BSP_FRAMEBUFFER_H

#include <ewoksys/fbinfo.h>

fbinfo_t* bsp_get_fbinfo(void);
int32_t bsp_fb_init(uint32_t w, uint32_t h, uint32_t dep);

#endif
