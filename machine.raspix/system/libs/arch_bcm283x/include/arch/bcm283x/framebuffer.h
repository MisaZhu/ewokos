#ifndef BCM283X_FRAMEBUFFER_H
#define BCM283X_FRAMEBUFFER_H

#include <ewoksys/fbinfo.h>

fbinfo_t* bcm283x_get_fbinfo(void);
int32_t bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep);

#endif
