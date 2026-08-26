#ifndef DISP_INFO_H
#define DISP_INFO_H

#include <stdint.h>
#include <ewoksys/ewokdef.h>

typedef struct  {
	uint32_t width, height;
	uint32_t vwidth, vheight; /* virtual? */
	uint32_t pitch; /* byte count in a row */
	uint32_t depth; /* bits per pixel */
	uint32_t xoffset, yoffset;
	ewokos_addr_t pointer; /* virtual address mapped into the caller (SYS_MEM_MAP) */
	uint32_t size, size_max;
	int32_t  dma_id;
	ewokos_addr_t phy_base; /* CPU physical address of the framebuffer */
	ewokos_addr_t bus_base; /* address a DMA master must use to reach it;
	                           equals phy_base unless the DMA path has an
	                           address-translation window (e.g. RP1 on Pi5
	                           reads host RAM over the PCIe inbound window,
	                           so bus_base = phy_base + 0x1000000000). */
} disp_info_t;

#endif
