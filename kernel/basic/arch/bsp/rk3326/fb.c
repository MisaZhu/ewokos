#include <dev/fb.h>
#include <graph/graph.h>
#include <bcm283x/framebuffer.h>
#include <kstring.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


int argv2rgb(uint8_t  *out,  uint32_t *in , int w, int h)
{
	for(int i = 0; i < w*h; i++){
		 register uint32_t color = *in++;
		 *out++=color ;
		 *out++=color >> 8;
		 *out++=color >> 16;
	}	

	return 0;
}


int32_t fb_init(uint32_t w, uint32_t h, fbinfo_t* fbinfo) {
	(void)w;
	(void)h;

	fbinfo->width = 640;
	fbinfo->height = 480;
	fbinfo->vwidth = 640;
	fbinfo->vheight = 480;
	fbinfo->depth = 24;
	fbinfo->pointer = 0x3de00000;
	fbinfo->size = 640 * 480 * 4;

	return 0;
}

void fb_flush32(uint32_t* g32, uint32_t w, uint32_t h) {
	argv2rgb((uint8_t*)0x3de00000, g32, w, h);
}
