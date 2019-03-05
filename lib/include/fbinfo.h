#ifndef FRAMEBUFFER_INFO_H
#define FRAMEBUFFER_INFO_H

#include <types.h>

typedef struct  {
	uint32_t width, height;
	uint32_t vwidth, vheight; /* virtual? */
	uint32_t pitch; /* byte count in a row */
	uint32_t depth; /* bits per pixel */
	uint32_t xoffset, yoffset;
	uint32_t pointer, size;
} FBInfoT;

#endif
