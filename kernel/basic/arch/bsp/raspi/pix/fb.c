#include <dev/fb.h>
#include <bcm283x/framebuffer.h>
#include <kstring.h>

int32_t fb_init(uint32_t w, uint32_t h, fbinfo_t* fbinfo) {
	if(bcm283x_fb_init(w, h, 32) != 0)
		return -1;
	memcpy(fbinfo, bcm283x_get_fbinfo(), sizeof(fbinfo_t));
	return 0;
}

void fb_flush32(uint32_t* g32, uint32_t w, uint32_t h) {
	(void)g32;
	(void)w;
	(void)h;
}