#ifndef BSP_BSP_G3D_H
#define BSP_BSP_G3D_H

#include <stdint.h>
#include <ewoksys/ewokdef.h>

/* portablegl 3D (V3D 3D-pipeline) hardware offload.  Only the raspi5
   back end implements these against the V3D GPU (via libvideocore's
   vc_pgl3d layer); every other machine declines both ops with the sticky
   G2D_ERR_NOT_SUPPORTED (-2) so a portablegl client latches the 3D path
   off and keeps its own built-in CPU/NEON rasterizer.

   fmt/mode/fs_id/depth_func use the G2D_PGL_* values (see g2dclient.h);
   *_phy are resolved contig physical bases the GPU works on directly.
   ext/tex are opaque pointers to the caller's g2d_pgl_ext_t /
   g2d_pgl_tex_t (NULL leaves the matching state at its memset default).
   Both ops return the g2d result codes: G2D_OK (0), G2D_ERR_FAILED (-1,
   transient) or G2D_ERR_NOT_SUPPORTED (-2, sticky). */

/* bring up the 3D pipeline before first use (idempotent).  returns 0 when
   the 3D path is usable, non-zero when it is not.  the raspi5 back end
   also re-checks this lazily inside bsp_g3d_clear/bsp_g3d_draw, so an
   unsupported platform simply declines every op. */
int32_t bsp_g3d_init(void);

/* full-frame clear of the w x h render target backed by fb_phy/contig. */
int32_t bsp_g3d_clear(uint32_t* fb, ewokos_addr_t fb_phy, uint8_t contig,
			int32_t w, int32_t h, uint32_t stride, uint32_t fmt,
			uint32_t color, int32_t rb_swap);

/* draw nverts post-vertex-shader primitives into the render target.
   prog_handle >= 0 selects an on-device-compiled program (uploaded via
   bsp_g3d_program); `unif` then points to num_uniforms u32 per-draw GL
   uniform values the back end's QUNIFORM interpreter reads (NULL when
   num_uniforms == 0), and `ctex` points to a g2d_pgl_ctex_t describing up
   to G2D_PGL_MAX_TEX_UNITS sampler-unit texture bindings the compiled FS
   may sample from (NULL when the program references no texture unit).
   prog_handle < 0 keeps the fixed-function fs_id path and ctex is ignored.
   Only the raspi5 back end honours the compiled path; the six stubs ignore
   all four new arguments and decline with -2. */
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
			const void* ctex);

/* Upload a compiled shader program: `blob` points to a serialized
   g2d_pgl_program_blob_t header immediately followed by its QPU/uniform
   payload, `size` bytes total (a CPU-side buffer - no GPU visibility needed).
   The 3D back end copies the QPU into GPU memory and caches the program.
   Returns a non-negative program handle (referenced by a later draw) or a
   negative g2d result code: G2D_ERR_FAILED (-1, transient) or
   G2D_ERR_NOT_SUPPORTED (-2, sticky - no 3D path, or the program is out of
   range so the client keeps it on the CPU).  Only the raspi5 back end
   implements this; every other machine declines with -2. */
int32_t bsp_g3d_program(const void* blob, uint32_t size);

/* Release a program handle returned by bsp_g3d_program: its cache slot and
   GPU memory go back for reuse.  g2dd calls this for every program whose
   owning client has exited (the back end caches only a handful of programs,
   so without it a relaunched app soon finds the cache full and falls back to
   the CPU).  Returns 0, or -2 for an unknown / already-free handle and on
   every machine without a 3D back end. */
int32_t bsp_g3d_program_free(int32_t handle);

#endif
