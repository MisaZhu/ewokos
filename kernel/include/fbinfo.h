#ifndef FRAMEBUFFER_INFO_H
#define FRAMEBUFFER_INFO_H

#include <types.h>

enum {
	RES_640x480 = 0,
	RES_800x600,
	RES_1024x768,
	RES_1920x1080
};

typedef struct  {
	uint32_t width, height;
	uint32_t vwidth, vheight; /* virtual? */
	uint32_t pitch; /* byte count in a row */
	uint32_t depth; /* bits per pixel */
	uint32_t xoffset, yoffset;
	uint32_t pointer, size;
} fbinfo_t;

#endif
