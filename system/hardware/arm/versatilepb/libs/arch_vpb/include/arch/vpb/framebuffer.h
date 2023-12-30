#ifndef VPB_FRAMEBUFFER_H
#define VPB_FRAMEBUFFER_H

#include <ewoksys/fbinfo.h>

fbinfo_t* vpb_get_fbinfo(void);
int32_t vpb_fb_init(uint32_t w, uint32_t h, uint32_t dep);

#endif
