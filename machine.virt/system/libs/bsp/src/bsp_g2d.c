#include <bsp/bsp_g2d.h>
#include <g2d_arch.h>

/* thin dispatch layer: every operation is implemented by the platform's
   arch_g2d_* back end (NEON software engine on virt, a hardware 2D engine
   on platforms that provide one). the *_contig flags tell the back end
   when a buffer is physically contiguous, and *_phy carries the resolved
   physical base so a hardware engine can work on physical addresses. */

int32_t bsp_g2d_init(void) {
	return arch_g2d_init();
}

int32_t bsp_g2d_fill(uint32_t* argb, ewokos_addr_t argb_phy, uint8_t contig, int32_t argb_w, int32_t argb_h,
			int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	return arch_g2d_fill(argb, argb_phy, contig, argb_w, argb_h, x, y, w, h, color);
}

int32_t bsp_g2d_fill_alpha(uint32_t* argb, int32_t argb_w, int32_t argb_h,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	return arch_g2d_fill_alpha(argb, argb_w, argb_h, x, y, w, h, color);
}

int32_t bsp_g2d_blt(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	return arch_g2d_blt(argb_src, src_phy, src_contig, src_w, src_h, sx, sy, sw, sh,
			argb_dst, dst_phy, dst_contig, dst_w, dst_h, dx, dy, dw, dh);
}

/* software-only back end: the arch engine works on virtual pointers
   and this platform has no hardware 2d engine able to write a raw
   physical range, so the scan-out push is declined; the caller
   (displayd flush_g2d) falls back to the driver's cpu flush */
int32_t bsp_g2d_blt_phy(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			ewokos_addr_t dst_phy, uint32_t dst_size, int32_t dst_w, int32_t dst_h,
			uint32_t dst_pitch,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	(void)argb_src; (void)src_phy; (void)src_contig; (void)src_w; (void)src_h;
	(void)sx; (void)sy; (void)sw; (void)sh;
	(void)dst_phy; (void)dst_size; (void)dst_w; (void)dst_h;
	(void)dst_pitch;
	(void)dx; (void)dy; (void)dw; (void)dh;
	return -1;
}

int32_t bsp_g2d_blt_alpha(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	return arch_g2d_blt_alpha(argb_src, src_phy, src_contig, src_w, src_h, sx, sy, sw, sh,
			argb_dst, dst_phy, dst_contig, dst_w, dst_h, dx, dy, dw, dh, alpha);
}

int32_t bsp_g2d_scale_to(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h) {
	return arch_g2d_scale_to(argb_src, src_phy, src_contig, src_w, src_h,
			argb_dst, dst_phy, dst_contig, dst_w, dst_h);
}

int32_t bsp_g2d_rotated_size(int32_t src_w, int32_t src_h, int32_t degree,
			int32_t* dst_w, int32_t* dst_h) {
	return arch_g2d_rotated_size(src_w, src_h, degree, dst_w, dst_h);
}

int32_t bsp_g2d_rotate(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h, int32_t degree) {
	return arch_g2d_rotate(argb_src, src_phy, src_contig, src_w, src_h,
			argb_dst, dst_phy, dst_contig, dst_w, dst_h, degree);
}
