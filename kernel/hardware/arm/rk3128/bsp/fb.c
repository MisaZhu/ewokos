#include <dev/fb.h>
#include <graph/graph.h>
#include <kstring.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int32_t fb_init(uint32_t w, uint32_t h, fbinfo_t* fbinfo) {
	(void)w;
	(void)h;

	fbinfo->width = 1024;
	fbinfo->height = 600;
	fbinfo->vwidth = 1024;
	fbinfo->vheight = 600;
	fbinfo->depth = 32;
	fbinfo->pointer = 0x6dd00000;
	fbinfo->size = 1024 * 600 * 4;

	return 0;
}

void fb_flush32_bsp(uint32_t* g32, uint32_t w, uint32_t h) {
	(void)g32;
	(void)w;
	(void)h;
}
