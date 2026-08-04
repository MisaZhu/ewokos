#ifndef FRAMEBUFFER_INFO_H
#define FRAMEBUFFER_INFO_H

#include <stdint.h>
#include <ewoksys/ewokdef.h>

typedef struct  {
	uint32_t width, height;
	uint32_t vwidth, vheight; /* virtual? */
	uint32_t pitch; /* byte count in a row */
	uint32_t depth; /* bits per pixel */
	uint32_t xoffset, yoffset;
	ewokos_addr_t pointer;
	uint32_t size, size_max;
	int32_t  dma_id;
	ewokos_addr_t phy_base;
	ewokos_addr_t bus_base;
} fbinfo_t;

#endif
