#include <bsp/bsp_fb.h>
#include <stdint.h>
#include <sysinfo.h>
#include <string.h>
#include <arch/bcm283x/framebuffer.h>

fbinfo_t* bsp_get_fbinfo(void) {
  return bcm283x_get_fbinfo();
}

int32_t bsp_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
  return bcm283x_fb_init(w, h, dep);
}
