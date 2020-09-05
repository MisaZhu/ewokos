#ifndef BCM283X_FRAMEBUFFER_H
#define BCM283X_FRAMEBUFFER_H

#include <fbinfo.h>

int32_t bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep, fbinfo_t* fbinfo);

#endif
