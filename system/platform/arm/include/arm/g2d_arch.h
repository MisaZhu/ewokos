#ifndef ARCH_G2D_H
#define ARCH_G2D_H

#include <stdint.h>
#include <ewoksys/ewokdef.h>

/* initialize the 2D engine before first use. safe to call multiple
   times. returns 0 on success. */
int32_t arch_g2d_init(void);

/* *_phy: resolved physical base of the buffer when the matching
   *_contig flag is set, 0 otherwise; consumed by hardware 2d engines
   that work on physical addresses */
int32_t arch_g2d_fill(uint32_t* argb, ewokos_addr_t argb_phy, uint8_t contig, int32_t argb_w, int32_t argb_h,
			int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

/* scalar cpu alpha fill of a sub-rect, clipped to the buffer bounds:
   exact per-pixel access, no alignment/contiguity/simd requirements;
   same blend math as arch_g2d_blt_alpha. alpha == 0 is a no-op. */
int32_t arch_g2d_fill_alpha(uint32_t* argb, int32_t argb_w, int32_t argb_h,
			int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

int32_t arch_g2d_blt(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh);

int32_t arch_g2d_blt_alpha(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);

/* scalar cpu 1:1 copy or blend of a sub-rect, clipped against both
   buffer bounds: exact per-pixel access, no alignment/contiguity/simd
   requirements. use_alpha == 0 is a plain copy; otherwise the same
   blend math as arch_g2d_blt_alpha (effective alpha
   (src_a * alpha) >> 8, then the /255 blend). */
int32_t arch_g2d_blt_cpu(uint32_t* argb_src, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, uint8_t use_alpha, uint8_t alpha);

int32_t arch_g2d_scale_to(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h);

/* smallest size able to hold src_w x src_h rotated clockwise by degree
   (any angle). exact swap/keep for multiples of 90, rotated bounding
   box otherwise. */
int32_t arch_g2d_rotated_size(int32_t src_w, int32_t src_h, int32_t degree,
			int32_t* dst_w, int32_t* dst_h);

/* rotate the whole source surface clockwise by degree (any angle).
   dst must be at least the size given by arch_g2d_rotated_size(); for
   angles other than 0/90/180/270 pixels outside the rotated content
   become transparent.
   in-place (argb_src == argb_dst) is only valid for 0/180. */
int32_t arch_g2d_rotate(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h, int32_t degree);

#endif
