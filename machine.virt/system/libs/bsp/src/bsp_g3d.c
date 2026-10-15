/*
 * bsp_g3d.c - portablegl 3D offload stub.  This platform has no V3D
 * 3D-pipeline back end, so decline every op with the sticky
 * G2D_ERR_NOT_SUPPORTED (-2): a portablegl client latches the capability
 * off and keeps its own built-in CPU/NEON rasterizer.  Nothing is
 * submitted.  bsp_g3d_init() is a benign no-op.
 *
 * The literal -2 mirrors G2D_ERR_NOT_SUPPORTED (this file does not pull in
 * g2dclient.h); g2dd passes the code back to the client verbatim.
 */

#include <bsp/bsp_g3d.h>

int32_t bsp_g3d_init(void) {
	return 0;
}

int32_t bsp_g3d_clear(uint32_t* fb, ewokos_addr_t fb_phy, uint8_t contig,
		int32_t w, int32_t h, uint32_t stride, uint32_t fmt,
		uint32_t color, int32_t rb_swap) {
	(void)fb; (void)fb_phy; (void)contig; (void)w; (void)h;
	(void)stride; (void)fmt; (void)color; (void)rb_swap;
	return -2 /* G2D_ERR_NOT_SUPPORTED */;
}

int32_t bsp_g3d_draw(ewokos_addr_t fb_phy, uint8_t fb_contig,
		int32_t w, int32_t h, uint32_t stride, uint32_t fmt,
		uint32_t mode, uint32_t nverts,
		ewokos_addr_t pos_phy, uint8_t pos_contig,
		ewokos_addr_t col_phy, uint8_t col_contig,
		uint32_t fs_id, uint32_t color, int32_t rb_swap,
		uint32_t depth_func, int32_t depth_mask,
		int32_t blend_en, int32_t cull_en,
		ewokos_addr_t z_phy, uint8_t z_contig,
		uint32_t z_stride, uint8_t first_frame,
		uint32_t clear_rgba, float clear_z, const void* ext, const void* tex,
		int32_t prog_handle, const void* unif, uint32_t num_uniforms,
		const void* ctex) {
	(void)fb_phy; (void)fb_contig; (void)w; (void)h; (void)stride;
	(void)fmt; (void)mode; (void)nverts; (void)pos_phy; (void)pos_contig;
	(void)col_phy; (void)col_contig; (void)fs_id; (void)color;
	(void)rb_swap; (void)depth_func; (void)depth_mask; (void)blend_en;
	(void)cull_en;
	(void)z_phy; (void)z_contig; (void)z_stride;
	(void)first_frame; (void)clear_rgba; (void)clear_z; (void)ext; (void)tex;
	(void)prog_handle; (void)unif; (void)num_uniforms; (void)ctex;
	return -2 /* G2D_ERR_NOT_SUPPORTED */;
}

int32_t bsp_g3d_program(const void* blob, uint32_t size) {
	(void)blob; (void)size;
	return -2 /* G2D_ERR_NOT_SUPPORTED */;
}

int32_t bsp_g3d_program_free(int32_t handle) {
	(void)handle;
	return -2 /* G2D_ERR_NOT_SUPPORTED */;
}
