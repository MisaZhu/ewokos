#ifndef BSP_BSP_G2D_H
#define BSP_BSP_G2D_H

#include <stdint.h>
#include <ewoksys/ewokdef.h>

/* initialize the 2D engine before first use. safe to call multiple
   times. returns 0 on success. */
int32_t bsp_g2d_init(void);

/* contig/src_contig/dst_contig: != 0 when the buffer backing is
   physically contiguous (contig shm slab or dma memory), required by
   hardware 2d paths that work on physical addresses. *_phy carries the
   resolved physical base of the buffer when contig, 0 otherwise. */
int32_t bsp_g2d_fill(uint32_t* argb, ewokos_addr_t argb_phy, uint8_t contig, int32_t argb_w, int32_t argb_h,
			int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

/* scalar cpu alpha fill of a sub-rect, clipped to the buffer bounds:
   exact per-pixel access, no alignment/contiguity/simd requirements;
   same blend math as bsp_g2d_blt_alpha. alpha == 0 is a no-op. */
int32_t bsp_g2d_fill_alpha(uint32_t* argb, int32_t argb_w, int32_t argb_h,
			int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

int32_t bsp_g2d_blt(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh);

/* 1:1 blit of a clipped rect into a RAW PHYSICAL destination (e.g. the
   scan-out buffer): no dst virtual address exists in this process, the
   2d engine writes the physical range directly. dst_pitch is the row
   stride in bytes (>= dst_w*4, %4==0); [dst_phy, dst_phy+dst_size)
   must be physically contiguous ram. returns -1 when the back end
   cannot write the physical range: the caller falls back to its cpu
   flush path. */
int32_t bsp_g2d_blt_phy(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			ewokos_addr_t dst_phy, uint32_t dst_size, int32_t dst_w, int32_t dst_h,
			uint32_t dst_pitch,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh);

int32_t bsp_g2d_blt_alpha(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);

int32_t bsp_g2d_scale_to(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h);

/* smallest size able to hold src_w x src_h rotated clockwise by degree
   (any angle). exact swap/keep for multiples of 90, rotated bounding
   box otherwise. */
int32_t bsp_g2d_rotated_size(int32_t src_w, int32_t src_h, int32_t degree,
			int32_t* dst_w, int32_t* dst_h);

/* rotate the whole source surface clockwise by degree (any angle).
   dst must be at least the size given by bsp_g2d_rotated_size(); for
   angles other than 0/90/180/270 pixels outside the rotated content
   become transparent.
   in-place (argb_src == argb_dst) is only valid for 0/180. */
int32_t bsp_g2d_rotate(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h, int32_t degree);

#endif
