#ifndef G2D_G2D_CLIENT_H
#define G2D_G2D_CLIENT_H

#include <stdint.h>
#include <string.h>
#include <ewoksys/fsinfo.h>
#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	G2D_DEV_CNTL_FILL_RECT = 0,
	G2D_DEV_CNTL_BLIT,
	G2D_DEV_CNTL_BLIT_ALPHA,
	G2D_DEV_CNTL_ROTATE,
	G2D_DEV_CNTL_SCALE_TO,
	G2D_DEV_CNTL_GET_CLOCK,
	G2D_DEV_CNTL_BLIT_TO_PHY,
	G2D_DEV_CNTL_GAUSSIAN_BLUR,
	G2D_DEV_CNTL_ROTATE_TO_PHY,
	G2D_DEV_CNTL_FILL_TRIANGLE,
	G2D_DEV_CNTL_PGL_CLEAR,
	G2D_DEV_CNTL_PGL_DRAW,
	G2D_DEV_CNTL_PGL_PROGRAM,
	G2D_DEV_CNTL_GET_VERSION,
	/* OpenCL compute offload, appended last (never inserted mid-enum) so the
	   existing command values, and thus the client<->daemon wire protocol for
	   every other op, stay unchanged.  Only the raspi5 back end implements
	   these; every other machine declines all three with the sticky
	   G2D_ERR_NOT_SUPPORTED, so libOpenCL latches its GPU device off and
	   keeps the CPU device.  CL_PROGRAM uploads a mesa-compiled compute blob
	   (the SAME g2d_pgl_program_blob_t wire format, compute stage only) and
	   answers a non-negative program handle; CL_DISPATCH runs a work-group
	   grid against that handle; CL_PROGRAM_FREE releases it.  g2dd's synthetic
	   G2DD_STAT_FILL_ALPHA slot is defined relative to CL_PROGRAM_FREE (the
	   new last command), not GET_VERSION. */
	G2D_DEV_CNTL_CL_PROGRAM,
	G2D_DEV_CNTL_CL_DISPATCH,
	G2D_DEV_CNTL_CL_PROGRAM_FREE
};

/* portablegl GPU (V3D 3D-pipeline) offload.  Only the raspi5 back end
   implements these; every other machine declines both ops with the sticky
   G2D_ERR_NOT_SUPPORTED so a portablegl client latches off and keeps its
   own CPU rasterizer.  The constants below mirror the raspi5 vc_pgl3d back
   end's values exactly, so g2dd forwards them unchanged. */
enum {
	G2D_PGL_MODE_POINTS    = 0,
	G2D_PGL_MODE_LINES     = 1,
	G2D_PGL_MODE_TRIANGLES = 4
};
enum {
	G2D_PGL_FMT_BGR565 = 7,
	G2D_PGL_FMT_RGBA8  = 27
};
enum {
	G2D_PGL_FS_SOLID   = 0,
	G2D_PGL_FS_GOURAUD = 1,
	G2D_PGL_FS_TEX_MOD = 2
};
enum {
	G2D_PGL_DEPTH_NONE = 0,
	G2D_PGL_DEPTH_LESS,
	G2D_PGL_DEPTH_LEQUAL,
	G2D_PGL_DEPTH_GREATER,
	G2D_PGL_DEPTH_GEQUAL,
	G2D_PGL_DEPTH_ALWAYS,
	G2D_PGL_DEPTH_NEVER,
	G2D_PGL_DEPTH_EQUAL,
	G2D_PGL_DEPTH_NOTEQUAL
};
/* back-face-cull config carried in cull_en (0 = draw both facings) */
enum {
	G2D_PGL_CULL_NONE = 0,
	G2D_PGL_CULL_BACK_CCW,
	G2D_PGL_CULL_BACK_CW,
	G2D_PGL_CULL_FRONT_CCW,
	G2D_PGL_CULL_FRONT_CW,
	G2D_PGL_CULL_BOTH
};
/* maximum vertices per draw dispatch (the back end declines more).  Raised to
   20480 so a whole rigid mesh (e.g. the 19200-vertex Utah teapot) can ride ONE
   dispatch instead of many small ones: each dispatch pays a fixed whole-surface
   L2 clean+invalidate + IRQ wait + IPC round-trip independent of triangle
   count, so batching a mesh into a single dispatch is what makes the GPU path
   beat the CPU rasterizer.  Must stay >= the client's PGL_G3D_MAX_VERTS. */
#define G2D_PGL_MAX_VERTS 20480u

/* Maximum texture units a compiled FS may bind (iChannel0..iChannelN).
   Sized to cover the shadertoy set (max 3 channels in the_cave) with one
   slot of headroom.  Mirrors V3D_MAX_TEXTURE_SAMPLERS on the back end.
   Used by g2d_pgl_req_t's compiled-shader sampler-binding block below. */
#define G2D_PGL_MAX_TEX_UNITS   4u

/* result codes every g2dclient call below answers with (the driver
   sends the same codes back on the wire):
   G2D_OK                 the operation completed.
   G2D_ERR_FAILED         the operation was attempted but failed (bad
                          canvas, attach failure, back end error, ...).
                          transient: a different request may succeed.
   G2D_ERR_NOT_SUPPORTED  the driver has no such capability (unknown
                          command, or the back end declined the op
                          itself). sticky: asking again cannot change
                          the answer, so the caller should run its own
                          fallback and stop offloading that op. */
#define G2D_OK                0
#define G2D_ERR_FAILED        (-1)
#define G2D_ERR_NOT_SUPPORTED (-2)
/* Submitted GPU work may still own its buffers. These terminal results must
   never trigger CPU replay, buffer reuse or release without a GPU reset. */
#define G2D_ERR_TIMEOUT       (-3)
#define G2D_ERR_MMU_FAULT     (-4)

/* Retain this process and its buffers on a terminal GPU result. There is no
   reset/recovery protocol yet; rendering clients must stop until reboot.
   Nonterminal results return immediately and may use software fallback. */
void g2d_quiesce_on_fault(int result);

enum {
	G2D_FMT_ARGB8888 = 0
};

/* common clockwise rotation degrees, any angle is accepted and
   normalized to [0, 360) by the driver */
enum {
	G2D_ROTATE_0 = 0,
	G2D_ROTATE_90 = 90,
	G2D_ROTATE_180 = 180,
	G2D_ROTATE_270 = 270
};

typedef struct {
	int32_t x;
	int32_t y;
	int32_t w;
	int32_t h;
} g2d_rect_t;

/* a canvas is a keyed shm segment of w*h ARGB8888 pixels shared with
   the driver: the client writes pixels in, the driver attaches to the
   same segment and operates on it in place. the driver owns no canvas
   of its own, every request carries the canvases it works on.
   contig != 0 tells the driver the shm backing is physically
   contiguous (allocated with IPC_CONTIG), needed by hardware 2d paths
   that work on physical addresses; phy may carry the resolved physical
   base of the segment, or 0 (the usual case) to let the driver resolve
   it with shm_contig_phy_addr when it first attaches the segment - the
   driver caches the attach, so a client-side translation would be a
   syscall per canvas per request for nothing.
   when dma != 0 the canvas lives in dma memory instead: addr is the
   buffer address returned by dma_alloc() (a vaddr in the sys_dma v
   window, mapped only into the allocator; the g2d driver mem-maps it
   on attach) and shm_id is ignored. */
typedef struct {
	int32_t shm_id;
	uint8_t dma;      /* 0: shm canvas (shm_id), !=0: dma canvas (addr) */
	uint8_t contig;   /* shm backing physically contiguous (IPC_CONTIG) */
	uint8_t reserved[2];
	uint32_t size;  /* segment size in bytes, must be >= w*h*4 */
	uint32_t w;
	uint32_t h;
	ewokos_addr_t addr; /* dma address of the buffer when dma != 0 */
	ewokos_addr_t phy;  /* physical base of the buffer when contig != 0 */
} g2d_canvas_t;

typedef struct {
	g2d_canvas_t dst;
	g2d_rect_t rect;
	uint32_t color;
} g2d_fill_req_t;

/* solid/alpha filled triangle (x1,y1)-(x2,y2)-(x3,y3) rasterized into the
   dst canvas with the same scanline fill the software reference uses
   (sort-by-Y, two-slope edge walk). the driver clips the rasterized spans
   to the canvas bounds; a triangle whose bounding box is fully outside the
   canvas is refused with G2D_OK (nothing to draw). color carries the blend
   alpha in its high byte (0xff = opaque). */
typedef struct {
	g2d_canvas_t dst;
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	int32_t x3;
	int32_t y3;
	uint32_t color;
} g2d_triangle_req_t;

/* rotates the src canvas clockwise by degree (any angle, normalized to
   [0, 360) by the driver) into the dst canvas. 90/270 swap dimensions,
   other angles grow to the rotated bounding box; the driver rejects the
   request when the dst canvas size does not match the rotated size. */
typedef struct {
	g2d_canvas_t src;
	g2d_canvas_t dst;
	int32_t rotate;
} g2d_rotate_req_t;

/* scales the src canvas into the dst canvas (nearest neighbor). */
typedef struct {
	g2d_canvas_t src;
	g2d_canvas_t dst;
} g2d_scale_to_req_t;

/* 1:1 blit of a src crop into a RAW PHYSICAL destination (e.g. the
   scan-out buffer): no dst canvas exists because the buffer is not a
   shm segment. the physical range must be contiguous driver-visible
   ram (the hardware 2d has no mmu); the driver rejects anything
   outside the validated ram windows or the declared size. no alpha,
   scaling or rotation - 1:1 copy only. pitch is the dst row stride in
   bytes. */
typedef struct {
	g2d_canvas_t src;
	int32_t sx;
	int32_t sy;
	int32_t sw;
	int32_t sh;
	ewokos_addr_t dst_phy;
	uint32_t dst_size;  /* bytes available at dst_phy */
	int32_t dst_w;      /* visible surface geometry */
	int32_t dst_h;
	uint32_t pitch;     /* row stride in bytes (>= dst_w*4, %4==0) */
	int32_t dx;
	int32_t dy;
	int32_t dw;
	int32_t dh;
} g2d_blit_to_phy_req_t;

/* right-angle rotation (90/180/270) of a src crop straight into a RAW
   PHYSICAL destination (the scan-out of a rotated panel): same dst
   model as g2d_blit_to_phy_req_t. the rotated crop lands with its
   top-left at (dx,dy); its size is the rotated (sw,sh), so the request
   carries no dst size. the driver clips nothing here - a crop outside
   the src canvas, or a landing rect outside the dst geometry, fails
   (G2D_ERR_FAILED) because a partial rotate would land in the wrong
   place. G2D_ERR_NOT_SUPPORTED (sticky) when the back end has no such
   path; a geometry the engine declines answers G2D_ERR_FAILED without
   touching dst, so the caller runs its own rotate for that rect. */
typedef struct {
	g2d_canvas_t src;
	int32_t sx;
	int32_t sy;
	int32_t sw;
	int32_t sh;
	ewokos_addr_t dst_phy;
	uint32_t dst_size;  /* bytes available at dst_phy */
	int32_t dst_w;      /* visible surface geometry */
	int32_t dst_h;
	uint32_t pitch;     /* row stride in bytes (>= dst_w*4, %4==0) */
	int32_t dx;
	int32_t dy;
	int32_t rotate;     /* clockwise degrees, normalized by the driver */
} g2d_rotate_to_phy_req_t;

typedef struct {
	g2d_canvas_t dst;
	g2d_canvas_t src;
	int32_t sx;
	int32_t sy;
	int32_t sw;
	int32_t sh;
	int32_t dx;
	int32_t dy;
	int32_t dw;
	int32_t dh;
	uint8_t alpha;
	uint8_t reserved[3];
	/* rotates the cropped clockwise by degree before scaling it into
	   the dst rect (any angle, normalized to [0, 360) by the driver) */
	int32_t rotate;
} g2d_blit_req_t;

/* GPU-only Gaussian-style blur, in place. The driver clips rect to dst;
   an empty rect or non-positive radius returns FAILED. Geometry/radius
   eligibility and the filter are backend-specific. Exact backends use
   fixed Q16 weights at radii 1..4 (sigma=radius/2), composing larger radii.
   Q8B prioritizes throughput with a bilinear downsample/upsample cascade,
   radii 1..64, including cropped rectangles on a 64-byte-aligned canvas
   pitch. Its pixels need not match the CPU Q16 kernel.
   Both canvases must be GPU-visible. For a clipped rect w*h on canvas
   pitch dst.w*4, tmp needs at least (h-1)*dst.w*4+w*4 bytes, with logical
   geometry w*h. A backend can decline if its scratch layout will not fit.
   A replayable FAILED leaves dst unchanged; tmp is disposable. There is
   no driver CPU fallback. Submitted timeouts/faults are terminal: retain
   both buffers and never replay on the CPU. */
typedef struct {
	g2d_canvas_t dst;
	g2d_canvas_t tmp;
	g2d_rect_t rect;
	int32_t radius;
} g2d_gaussian_blur_req_t;

/* Extended per-dispatch fixed-function state (Phase B2c): the scissor test
   window, polygon offset and colour write mask.  Carried as a sub-struct so
   bsp_g3d_draw grows by ONE opaque pointer instead of eight scalars: the
   full definition lives here (g2dd and the raspi5 back end include this
   header), while the six stub bsp layers only forward-declare the tag
   `struct g2d_pgl_ext_s` and ignore the pointer.  Every field defaults to zero
   through the request memset, which is exactly the "feature off / write all
   four channels" state, so an older caller that never sets ext is unaffected.
   The V3D 7.1 encodings these drive (colour-mask bit polarity, clip-window
   y-origin, depth-offset f187 scaling) are confirmed on the Pi5 in Phase G. */
typedef struct g2d_pgl_ext_s {
	int32_t  scissor_x;   /* clip window left, framebuffer pixels */
	int32_t  scissor_y;   /* clip window bottom (y down from the top edge) */
	int32_t  scissor_w;   /* scissor_w/h == 0 -> scissor off (full frame) */
	int32_t  scissor_h;
	uint8_t  poly_enable; /* polygon offset applies to this primitive */
	float    poly_factor; /* Depth Offset Factor (sent as f187) */
	float    poly_units;  /* Depth Offset Units (sent as f187) */
	uint32_t color_mask;  /* RGBA write-enable, GL order: bit0=R,1=G,2=B,3=A;
	                         0xF (or 0 from an old caller) = write all four */
	/* Optional compute-raster dispatch window.  These fields are consumed only
	   by the Q8B Adreno pixel-owned path; zero width/height keeps the historic
	   full-frame launch.  Coordinates are framebuffer rows (top-left origin),
	   matching the clipRect already carried in the shader uniform. */
	int32_t  dispatch_x;
	int32_t  dispatch_y;
	int32_t  dispatch_w;
	int32_t  dispatch_h;
} g2d_pgl_ext_t;

/* Phase D texture state passed to bsp_g3d_draw as an opaque pointer
   (mirrors g2d_pgl_ext_t so the six stub bsp layers only forward-declare the
   tag and ignore the pointer; the raspi5 back end fills its
   vc_pgl3d_draw_req_t tex fields verbatim).  tex_phy is the resolved physical
   base of the linear RGBA8 texture image (contig or dma); uv_phy is the
   resolved physical base of the per-vertex uv float3 attribute array.  Both
   must be non-zero for a TEX_MOD draw or the back end declines. */
typedef struct g2d_pgl_tex_s {
	ewokos_addr_t tex_phy;    /* linear RGBA8 texture image (row pitch tex_stride) */
	uint8_t       tex_contig;
	uint32_t      tex_stride; /* texture row pitch, bytes (0 -> derive w*4) */
	uint32_t      tex_w;      /* texture width, texels */
	uint32_t      tex_h;      /* texture height, texels */
	ewokos_addr_t uv_phy;     /* per-vertex uv float3 attribute (stride 12) */
	uint8_t       uv_contig;
} g2d_pgl_tex_t;

/* WS4 compiled-shader sampler-binding state passed to bsp_g3d_draw as an
   opaque pointer (mirrors g2d_pgl_tex_t so the six stub bsp layers only
   forward-declare the tag and ignore the pointer; the raspi5 back end
   forwards each entry into vc_pgl3d_draw_req_t's compiled-tex fields
   verbatim).  Each entry describes one texture unit's resolved physical
   base + geometry; ctex_phy[i] == 0 means unit i is unbound (any sample
   the FS makes from that unit returns whatever the driver leaves in the
   TMU config - safe black for shaders that never sample it).  The images
   are RGBA8 in the V3D texture memory layout named by ctex_tiling[i]
   (enum v3d_tex_tiling: 0 raster, 1 LT, 2/3 UBLINEAR, 4/5 UIF no-xor/xor,
   with ctex_ub_pad[i] the UIF level_0_ub_pad), contig or dma so the GPU
   can address them by physical base; ctex_stride[i] is the allocated row
   pitch of the (padded) image, 0 lets the driver derive ctex_w[i]*4.
   ctex_ub_pad[i] carries the UIF level_0_ub_pad in its low nibble
   (G2D_PGL_CTEX_UB_PAD_MASK) and the unit's sampler filter in the high
   nibble (G2D_PGL_CTEX_FILTER_*), packed into one byte so the request
   layout stays byte-identical to older statically-linked clients (which
   leave the high nibble 0 = NEAREST, the historic fixed sampler).
   Only meaningful when the draw carries a compiled prog_handle >= 0. */
#define G2D_PGL_CTEX_UB_PAD_MASK    0x0Fu
#define G2D_PGL_CTEX_FILTER_SHIFT   4u
#define G2D_PGL_CTEX_FILTER_NEAREST 0u   /* mag/min NEAREST (default)      */
#define G2D_PGL_CTEX_FILTER_LINEAR  1u   /* mag/min LINEAR (TMU bilinear)  */
#define G2D_PGL_CTEX_FILTER_OF(ub_pad) \
	(((uint32_t)(ub_pad) >> G2D_PGL_CTEX_FILTER_SHIFT) & 0x0Fu)
#define G2D_PGL_CTEX_UB_PAD_PACK(ub_pad, filter) \
	((uint8_t)(((ub_pad) & G2D_PGL_CTEX_UB_PAD_MASK) | \
	           (((filter) & 0x0Fu) << G2D_PGL_CTEX_FILTER_SHIFT)))
/* Explicit sampler state for mip-aware clients.  ctex_levels == 0 denotes a
 * legacy single-level request, whose mag/min filter comes from ub_pad's high
 * nibble.  Otherwise levels is 1..15 and these three bits are authoritative. */
#define G2D_PGL_CTEX_SAMPLER_MAG_LINEAR 0x01u
#define G2D_PGL_CTEX_SAMPLER_MIN_LINEAR 0x02u
#define G2D_PGL_CTEX_SAMPLER_MIP_LINEAR 0x04u
#define G2D_PGL_CTEX_SAMPLER_PACK(mag, min, mip) \
	((uint8_t)(((mag) ? G2D_PGL_CTEX_SAMPLER_MAG_LINEAR : 0u) | \
	           ((min) ? G2D_PGL_CTEX_SAMPLER_MIN_LINEAR : 0u) | \
	           ((mip) ? G2D_PGL_CTEX_SAMPLER_MIP_LINEAR : 0u)))
typedef struct g2d_pgl_ctex_s {
	ewokos_addr_t ctex_phy[G2D_PGL_MAX_TEX_UNITS];
	uint8_t       ctex_contig[G2D_PGL_MAX_TEX_UNITS];
	uint32_t      ctex_stride[G2D_PGL_MAX_TEX_UNITS];
	uint32_t      ctex_w[G2D_PGL_MAX_TEX_UNITS];
	uint32_t      ctex_h[G2D_PGL_MAX_TEX_UNITS];
	uint8_t       ctex_tiling[G2D_PGL_MAX_TEX_UNITS];
	uint8_t       ctex_ub_pad[G2D_PGL_MAX_TEX_UNITS];
	/* Cubemap / 2D-array support: ctex_depth[i] is the array layer count
	   (0 or 1 = a plain single-layer 2D image; 6 = a cubemap staged as a
	   6-layer 2D array, which is how mesa lowers a samplerCube TMU fetch).
	   ctex_arrstride64[i] is the per-layer tiled size in 64-byte units
	   (array_stride_64_byte_aligned in the V3D texture-state record); the
	   layers are stored contiguously at phy + layer*arrstride64*64.  Both
	   are 0 for a plain 2D image, so its record stays byte-identical. */
	uint8_t       ctex_depth[G2D_PGL_MAX_TEX_UNITS];
	uint32_t      ctex_arrstride64[G2D_PGL_MAX_TEX_UNITS];
	uint8_t       ctex_levels[G2D_PGL_MAX_TEX_UNITS];
	uint8_t       ctex_sampler[G2D_PGL_MAX_TEX_UNITS];
} g2d_pgl_ctex_t;

/* portablegl GPU offload request, used by BOTH PGL_CLEAR and PGL_DRAW.
   dst is the render target (an RGBA8/BGR565 contig-shm or dma canvas;
   modelled w x h so the driver's size >= w*h*4 attach check holds for
   RGBA8).  PGL_CLEAR uses only dst/fmt/color/stride/rb_swap.  PGL_DRAW
   also carries the CPU-computed post-vertex-shader data as two further
   canvases so the driver resolves their physical bases zero-copy:
     pos - float3 clip-space positions, nverts*12 bytes, modelled as a
           w=nverts h=3 canvas (nverts*3*4 == nverts*12);
     col - byte4 RGBA8 per vertex, nverts*4 bytes, modelled as a
           w=nverts h=1 canvas (nverts*1*4 == nverts*4).
   Both must be contig so the GPU can address them by physical base; a
   non-contig vertex buffer is declined.  nverts <= G2D_PGL_MAX_VERTS.
   The state subset (depth_func/depth_mask/blend_en/cull_en) is carried for
   the protocol; the raspi5 back end renders depth test/write (tile-local Z)
   and back-face cull for the built-in solid/gouraud shaders and declines
   blending (the fixed-function frag has no blend stage). */
typedef struct {
	g2d_canvas_t dst;
	g2d_canvas_t pos;
	g2d_canvas_t col;
	g2d_canvas_t z;        /* persistent Z32F depth surface (size 0 = none) */
	uint32_t mode;       /* G2D_PGL_MODE_* */
	uint32_t nverts;
	uint32_t fmt;        /* G2D_PGL_FMT_* */
	uint32_t fs_id;      /* G2D_PGL_FS_* */
	uint32_t color;      /* solid colour / clear colour, 0xAABBGGRR */
	uint32_t stride;     /* render-target row pitch, bytes */
	uint32_t z_stride;   /* Z-surface row pitch, bytes (w*4); 0 -> derive */
	uint32_t depth_func; /* G2D_PGL_DEPTH_* */
	uint32_t clear_rgba; /* colour the frame's first dispatch clears to */
	float    clear_z;    /* depth the frame's first dispatch clears to */
	uint8_t depth_mask;  /* depth writes enabled */
	uint8_t blend_en;    /* blending enabled */
	uint8_t cull_en;     /* G2D_PGL_CULL_* config code (0 = no cull) */
	uint8_t rb_swap;     /* swap R/B on store (BGR565 panel) */
	uint8_t first_frame; /* non-zero: first dispatch of the frame (clear) */
	g2d_pgl_ext_t ext;   /* Phase B2c scissor / polygon-offset / colour-mask */
	/* Phase D texture state (fs_id == G2D_PGL_FS_TEX_MOD only).  tex is a
	   linear RGBA8 texture image (contig or dma canvas; size >= tex_stride *
	   tex_h so the driver's attach check holds); uv is a per-vertex uv float3
	   canvas (nverts * 12 bytes, modelled w=nverts h=3, mirroring pos).
	   tex_w/tex_h/tex_stride describe the image (tex_stride == 0 lets the
	   driver derive w*4).  All default to zero through the request memset;
	   a TEX_MOD request with tex.size == 0 or uv.size == 0 is declined. */
	g2d_canvas_t tex;
	g2d_canvas_t uv;
	uint32_t tex_w;
	uint32_t tex_h;
	uint32_t tex_stride;
	/* --- WS4 compiled-shader draw reference ----------------------------
	   prog_handle selects an on-device-compiled program uploaded earlier via
	   G2D_DEV_CNTL_PGL_PROGRAM: a non-negative value is the daemon program
	   handle (the raspi5 back end builds the V3D shader-state record from the
	   cached prog_data and dispatches the compiled VS/FS QPU); the default -1
	   keeps the historical fixed-function path (fs_id SOLID/GOURAUD/TEX_MOD),
	   so every existing caller is unaffected.  When prog_handle >= 0, `unif`
	   is a contig-shm canvas carrying the per-draw GL uniform storage - an
	   array of num_uniforms u32 values that the daemon's QUNIFORM interpreter
	   indexes for QUNIFORM_UNIFORM slots (iGlobalTime / iResolution / ...);
	   it is modelled w=num_uniforms h=1 (num_uniforms*4 == size) so the
	   driver's size >= w*h*4 attach check holds, exactly like pos/col.  The
	   driver-computed uniforms (VIEWPORT_*_SCALE, TMU_CONFIG_*) are filled by
	   the back end from the render-target geometry and the bound textures, not
	   from this blob.  unif.size == 0 (num_uniforms == 0) is valid for a
	   program whose uniforms are all CONSTANT / driver-computed. */
	int32_t prog_handle;
	uint32_t num_uniforms;
	g2d_canvas_t unif;
	/* --- WS4 compiled-shader sampler bindings ---------------------------
	   A compiled FS may reference up to G2D_PGL_MAX_TEX_UNITS texture units
	   (iChannel0..iChannelN).  Each unit's QUNIFORM_TMU_CONFIG_P0/P1 pair
	   carries the unit index in the upper 8 bits of the data word, so the
	   daemon's expand_uniforms walks the FS uniform stream, extracts each
	   referenced unit index, and binds the corresponding texture canvas
	   below at that TMU slot.  A slot with tex.size == 0 is unbound: any
	   sample the FS makes from it returns whatever the driver leaves in
	   the TMU config (safe black for shaders that never sample it).
	   The images are RGBA8 in the V3D texture memory layout named by
	   ctex_tiling (enum v3d_tex_tiling; the TMU cannot sample a raster 2D
	   image, so the client tiles them with v3d_tex_store_2d) and contig or
	   dma so the GPU can address them by physical base; ctex_stride is the
	   allocated (padded) row pitch, 0 lets the driver derive tex_w * 4.
	   Only meaningful when prog_handle >= 0.  The fixed-function TEX_MOD
	   path continues to use the singular tex/uv fields above and never
	   touches this block.  All fields default to zero through the request
	   memset, so existing callers that never set them are unaffected. */
	g2d_canvas_t ctex[G2D_PGL_MAX_TEX_UNITS];
	uint32_t     ctex_w[G2D_PGL_MAX_TEX_UNITS];
	uint32_t     ctex_h[G2D_PGL_MAX_TEX_UNITS];
	uint32_t     ctex_stride[G2D_PGL_MAX_TEX_UNITS];
	uint8_t      ctex_tiling[G2D_PGL_MAX_TEX_UNITS];
	uint8_t      ctex_ub_pad[G2D_PGL_MAX_TEX_UNITS];
	/* Cubemap / 2D-array staging (see g2d_pgl_ctex_t): ctex_depth[i] is the
	   array layer count (0/1 = plain 2D, 6 = cubemap-as-2D-array) and
	   ctex_arrstride64[i] the per-layer tiled size in 64-byte units.  The
	   ctex[i] canvas size must then cover the WHOLE depth*arrstride64*64
	   allocation.  Both are 0 for a plain 2D image. */
	uint8_t      ctex_depth[G2D_PGL_MAX_TEX_UNITS];
	uint32_t     ctex_arrstride64[G2D_PGL_MAX_TEX_UNITS];
	uint8_t      ctex_levels[G2D_PGL_MAX_TEX_UNITS];
	uint8_t      ctex_sampler[G2D_PGL_MAX_TEX_UNITS];
} g2d_pgl_req_t;

/* --- On-device-compiled shader program upload (G2D_DEV_CNTL_PGL_PROGRAM) ----
   portablegl compiles a GLSL ES vertex+fragment pair to VideoCore V3D 7.1 QPU
   IN THE APP PROCESS (libv3dcompiler.a), then ships the result to g2dd ONCE via
   this command; g2dd forwards it to the raspi5 back end, which is the only
   entity that may copy the QPU into GPU-visible dma, build the V3D shader-state
   records and dispatch.  A later PGL_DRAW references the uploaded program by
   the integer handle this command returns, so the QPU is never re-sent per
   frame.

   The program travels as ONE serialized blob inside a contig-shm canvas (the
   same zero-copy model as the pos/col vertex canvases): a fixed
   g2d_pgl_program_blob_t header followed immediately by the variable payload.
   All fields are little-endian / naturally aligned; the payload is u32-aligned
   because the header size is a multiple of 4.  Payload order (each section
   present only when its count/size is non-zero):
     vs_qpu[vs_qpu_words]                 (u32 words: lo,hi per 64-bit instr)
     fs_qpu[fs_qpu_words]
     coord_qpu[coord_qpu_words]           (coordinate/binning shader)
     vs_uniform_contents[vs_num_uniforms] (enum quniform_contents as u32)
     vs_uniform_data[vs_num_uniforms]
     fs_uniform_contents[fs_num_uniforms]
     fs_uniform_data[fs_num_uniforms]
     coord_uniform_contents[coord_num_uniforms]
     coord_uniform_data[coord_num_uniforms]
     cs_qpu[cs_qpu_words]                 (VERSION 3: compute stage)
     cs_uniform_contents[cs_num_uniforms]
     cs_uniform_data[cs_num_uniforms]
   payload_bytes is the total byte length of all twelve sections, so the receiver
   can validate the canvas size without walking the counts.

   VERSION 2 adds the coordinate-shader (binning) stage: the SAME vertex NIR
   compiled with is_coord=true, so the V3D binner computes each vertex's
   clip-space position through the real model/view/projection transform instead
   of a fixed pass-through coord shader.  That is what lets arbitrary geometry
   (e.g. the teapot mesh) bin correctly; a pre-transformed full-screen quad
   could reuse the pass-through, a transformed mesh cannot.  The coord stage
   shares the render VS's uniform driver_locations (one linker assignment), so
   the client packs ONE per-draw uniform blob that feeds both the coord and the
   render-VS uniform expansion.

   VERSION 3 adds an OPTIONAL compute stage (cs_qpu_words == 0 when absent):
   the fragment program recompiled as a V3D compute kernel whose SSBO binding 0
   is the render target and whose per-draw uniform `v3d_cs_stride` is the
   target's row pitch in dwords.  A program carrying it is dispatched through
   the CSD as a full-frame grid (one invocation per pixel) instead of through
   the binner.  Its uniform driver_locations are assigned by the compute
   program's own linker pass, so the client looks a uniform name up in the
   COMPUTE stage too when packing the per-draw blob. */
#define G2D_PGL_PROGRAM_MAGIC   0x33445047u   /* 'G','P','D','3' little-endian */
#define G2D_PGL_PROGRAM_VERSION 6u
#define G2D_PGL_MAX_VS_ATTRS    64            /* mirrors V3D_MAX_VS_INPUTS */
#define G2D_PGL_MAX_FS_INPUTS   64            /* mirrors V3D_MAX_FS_INPUTS */

typedef struct {
	uint32_t magic;             /* G2D_PGL_PROGRAM_MAGIC */
	uint32_t version;           /* G2D_PGL_PROGRAM_VERSION */
	uint32_t vs_qpu_words;      /* 2 * VS QPU instruction count */
	uint32_t fs_qpu_words;      /* 2 * FS QPU instruction count */
	uint32_t vs_num_uniforms;
	uint32_t fs_num_uniforms;
	uint32_t payload_bytes;     /* bytes in the six payload sections */
	/* fragment-stage state the shader record + dispatch need */
	uint32_t fs_num_inputs;     /* varying inputs read (== VS outputs) */
	uint8_t  fs_needs_coords;   /* FS reads gl_FragCoord -> coord shader */
	uint8_t  fs_uses_center_w;  /* real_pixel_centre_mode record bit */
	uint8_t  fs_writes_z;       /* FS writes gl_FragDepth */
	uint8_t  fs_disable_ez;     /* early-Z must be disabled */
	uint8_t  fs_threads;        /* FS SIMD dispatch width */
	uint8_t  fs_single_seg;
	uint8_t  vs_threads;        /* VS SIMD dispatch width */
	uint8_t  vs_single_seg;
	/* vertex-stage VPM layout */
	uint32_t vs_vpm_input_size;
	uint32_t vs_vpm_output_size;
	uint32_t vs_vcm_cache_size;
	uint8_t  vs_separate_segments;
	uint8_t  reserved[3];
	/* vertex attribute -> driver_location map (fixed-size, mostly zero) */
	uint8_t  vs_vattr_sizes[G2D_PGL_MAX_VS_ATTRS / 4];
	int32_t  vs_driver_location_map[G2D_PGL_MAX_VS_ATTRS];
	/* fragment varying input slots (packed slot_and_component) */
	uint32_t fs_input_slots[G2D_PGL_MAX_FS_INPUTS];
	/* coordinate-shader (binning) stage: the same VS NIR compiled with
	   is_coord=true.  Carries its own QPU + uniform stream and VPM layout so
	   the binner applies the real MVP transform; the coord QPU/uniform payload
	   sections follow the FS uniform sections in the wire order documented
	   above.  The coord VPM sizes differ from the render VS's (the coord
	   shader writes only gl_Position, no varyings), so they are carried
	   separately for the shader-state record's coord VPM-segment fields. */
	uint32_t coord_qpu_words;     /* 2 * COORD QPU instruction count */
	uint32_t coord_num_uniforms;
	uint32_t coord_vpm_input_size;
	uint32_t coord_vpm_output_size;
	uint32_t coord_vcm_cache_size;
	uint8_t  coord_separate_segments;
	uint8_t  coord_threads;       /* COORD SIMD dispatch width */
	uint8_t  coord_single_seg;
	uint8_t  coord_reserved[1];
	/* VERSION 3: optional compute stage (see above); its three payload
	   sections follow the coord uniform sections. */
	uint32_t cs_qpu_words;        /* 2 * compute QPU instruction count, 0 = none */
	uint32_t cs_num_uniforms;
	uint32_t cs_local_size[3];    /* workgroup shape */
	uint32_t cs_shared_size;      /* bytes of shared memory (0 expected) */
	/* VERSION 5: bytes/thread of register-spill scratch the compute kernel
	 * needs (0 = no spill).  mesa's allocator spills heavy kernels (the wave
	 * faithful ocean emits QUNIFORM_SPILL_OFFSET / _SIZE_PER_THREAD); the daemon
	 * allocates a qpu_count*4*cs_spill_size DMA scratch buffer and resolves those
	 * two uniform slots from it so the kernel dispatches on the GPU instead of
	 * being declined to the CPU. */
	uint32_t cs_spill_size;
	uint8_t  cs_threads;          /* compute SIMD dispatch width */
	uint8_t  cs_single_seg;
	uint8_t  cs_is_geom;          /* geom kernel family: 1 gouraud, 2 textured,
	                                 3 instanced, 4 texcol, 5 = Vulkan user
	                                 shader (transpiler-generated two-pass
	                                 kernels, arbitrary location interface;
	                                 strides carried in cs2_vout_stride /
	                                 vk_attr_stride, VERSION 6) */
	/* VERSION 3 reserved byte, now assigned: raster-pass TRIANGLES PER
	 * WORKGROUP for geom programs.  0 = legacy grid of one workgroup per
	 * triangle; 64/256 = the one-lane-per-triangle kernels
	 * (geom_raster_cs_glsl_v4/v5), so the daemon dispatches
	 * ceil(ntris/cs_geom_tpw) workgroups.  Device-measured motivation: the
	 * V3D 7.1 CSD charges ~3us fixed overhead per workgroup, which at
	 * 6400 workgroups (teapot) was the entire ~19-20ms raster-pass cost.
	 * Old daemons ignore the byte and over-dispatch; the kernels' uTriCount
	 * guard keeps that correct (just slow).  Ignored for non-geom.
	 * uint16_t (NOT uint8_t): the V5 16x16 kernel covers 256 triangles per
	 * workgroup and 256 does not fit in a byte -- a uint8_t here silently
	 * truncated 256 to 0 and fell back to the one-workgroup-per-triangle grid.
	 * The Adreno twin (adreno_prog_blob_t.geom_tpw) is likewise uint16_t. */
	uint16_t cs_geom_tpw;
	/* VERSION 4: SECOND compute stage (cs2) for the two-pass geometry
	   rasterizer.  cs2 is the VERTEX-TRANSFORM pass (pass 1): one invocation
	   per vertex, applies uMVP (+ lighting for gouraud) and writes a packed
	   8-float/32-byte per-vertex record into the daemon-allocated `vout`
	   SSBO.  cs stays the RASTER pass (pass 2): one workgroup per triangle,
	   reads vout.  cs2_qpu_words == 0 when absent (single-pass or non-geom
	   program); its three payload sections follow the cs uniform sections in
	   the wire order.  The per-draw uniform blob is the CONCATENATION
	   [cs2/xform uniforms | cs/raster uniforms]; cs2_uniform_blob_dwords is
	   the split point (leading dwords feed cs2, the remainder feed cs). */
	uint32_t cs2_qpu_words;       /* 2 * xform compute QPU instruction count, 0 = none */
	uint32_t cs2_num_uniforms;
	uint32_t cs2_local_size[3];   /* workgroup shape */
	uint32_t cs2_shared_size;     /* bytes of shared memory (0 expected) */
	uint32_t cs2_uniform_blob_dwords; /* dwords of the per-draw blob for cs2 */
	uint8_t  cs2_threads;         /* compute SIMD dispatch width */
	uint8_t  cs2_single_seg;
	uint8_t  cs2_reserved[2];
	/* VERSION 6: per-vertex stream strides for the geom_kind == 5 (Vulkan
	   user-shader) two-pass kernels, in FLOATS.  cs2_vout_stride is the pass-1
	   vout record stride (8 + 4*ceil(nvarfloats/4)); vk_attr_stride is the
	   pass-1 attribute stream stride (the per-vertex float count the client
	   gathers).  The daemon uses them to size the pass-1 dst_span and to
	   validate the attr canvas span (nverts * vk_attr_stride * 4) in place of
	   the per-kind hard-coded pstride table.  Zero / ignored for kind != 5. */
	uint32_t cs2_vout_stride;
	uint32_t vk_attr_stride;
	/* variable payload follows immediately after this header */
} g2d_pgl_program_blob_t;

/* Upload request: `blob` is a contig-shm canvas whose first sizeof(header)
   bytes are a g2d_pgl_program_blob_t and whose remaining bytes are the
   payload; blob.size must be >= sizeof(header) + payload_bytes.  The back end
   copies the QPU into GPU dma and answers with a non-negative program handle
   (usable as g2d_pgl_req_t.fs_id-program selector on a later PGL_DRAW) or a
   negative G2D_ERR_* code. */
typedef struct {
	g2d_canvas_t blob;
} g2d_pgl_program_req_t;

/* --- OpenCL compute offload (G2D_DEV_CNTL_CL_*) ----------------------------
   libOpenCL's GPU device transpiles an OpenCL-C kernel to a GLSL-ES 3.10
   compute shader, compiles it IN THE APP PROCESS with the same mesa
   pglcompiler portablegl uses (pglc_compile_compute_raw + pglc_program_serialize),
   and ships the result to g2dd ONCE via CL_PROGRAM.  The blob is byte-for-byte
   the g2d_pgl_program_blob_t wire format above with ONLY the compute (cs)
   stage populated (vs/fs/coord word counts zero, cs_qpu_words != 0), so the
   raspi5 back end reuses its existing blob parser and QPU-into-dma path; a
   later CL_DISPATCH references the uploaded program by the integer handle
   CL_PROGRAM returns, so the QPU is never re-sent per launch.

   CL_PROGRAM travels the blob inside a contig-shm canvas exactly like
   g2d_pgl_program_req_t.blob (blob.size >= sizeof(header) + payload_bytes).
   It answers a non-negative program handle on the reply's second int (the
   same two-int pattern g2d_pgl_program uses) or a negative G2D_ERR_* code
   (G2D_ERR_NOT_SUPPORTED when the platform has no compute-upload path, so
   libOpenCL keeps the CPU device). */
typedef struct {
	g2d_canvas_t blob;
} g2d_cl_program_req_t;

/* Maximum __global buffer arguments one CL_DISPATCH carries as SSBO bindings.
   Each arg is a contig-shm or dma canvas so the GPU addresses it by physical
   base with zero copy (the same model as the pgl pos/col/unif canvases); the
   back end resolves each canvas' physical base into the compiled program's
   QUNIFORM_SSBO_OFFSET slots in binding order.  Kernels with more buffer args
   than this are run on libOpenCL's CPU device instead.  Sized to cover the
   demo set (vector_add/saxpy/matmul use <= 3) with generous headroom while
   keeping the request struct modest. */
#define G2D_CL_MAX_ARGS 16u

/* CL_DISPATCH request: run prog_handle over a wg_count[0] x wg_count[1] x
   wg_count[2] grid of work-groups, each of wg_size work-items (the
   local_size product, 1..256 -- the V3D CSD limit).  num_args entries of
   args[] are the __global buffer SSBOs in binding order (args[i].size == 0
   marks an unused trailing slot); num_uniforms / unif carry the packed
   scalar + __constant kernel arguments as an array of num_uniforms u32
   values in a contig-shm canvas modelled w=num_uniforms h=1 (so the
   driver's size >= w*h*4 attach check holds, exactly like the pgl unif
   canvas), which the back end's QUNIFORM interpreter indexes for the
   non-SSBO uniform slots.  unif.size == 0 (num_uniforms == 0) is valid for
   a kernel whose uniforms are all SSBO offsets.  Answers G2D_OK, a
   transient G2D_ERR_FAILED, or the sticky G2D_ERR_NOT_SUPPORTED. */
typedef struct {
	int32_t prog_handle;
	uint32_t wg_count[3];
	uint32_t wg_size;
	uint32_t num_args;
	g2d_canvas_t args[G2D_CL_MAX_ARGS];
	uint32_t num_uniforms;
	g2d_canvas_t unif;
} g2d_cl_dispatch_req_t;

/* CL_PROGRAM_FREE request: release a program handle uploaded via CL_PROGRAM.
   Carried as a bare int32 handle (no canvas), so the back end can reap the
   GPU dma the program owns.  Answers G2D_OK or a negative G2D_ERR_* code. */
typedef struct {
	int32_t prog_handle;
} g2d_cl_program_free_req_t;

static inline g2d_rect_t g2d_rect(int32_t x, int32_t y, int32_t w, int32_t h) {
	g2d_rect_t rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	return rect;
}

static inline g2d_canvas_t g2d_canvas(int32_t shm_id, uint32_t size, uint32_t w, uint32_t h, uint8_t contig) {
	g2d_canvas_t canvas;
	canvas.shm_id = shm_id;
	canvas.dma = 0;
	canvas.contig = contig;
	canvas.reserved[0] = 0;
	canvas.reserved[1] = 0;
	canvas.size = size;
	canvas.w = w;
	canvas.h = h;
	canvas.addr = 0;
	canvas.phy = 0;
	return canvas;
}

static inline g2d_canvas_t g2d_canvas_dma(ewokos_addr_t addr, uint32_t size, uint32_t w, uint32_t h) {
	g2d_canvas_t canvas;
	canvas.shm_id = 0;
	canvas.dma = 1;
	canvas.contig = 1; /* dma window memory is physically contiguous */
	canvas.reserved[0] = 0;
	canvas.reserved[1] = 0;
	canvas.size = size;
	canvas.w = w;
	canvas.h = h;
	canvas.addr = addr;
	canvas.phy = 0; /* resolved by the driver from the sys_dma window */
	return canvas;
}

/* Bind an on-device-compiled program (uploaded via g2d_pgl_program) to a draw
   request, together with its per-draw GL uniform storage.  `unif_shm` is the
   contig-shm segment holding num_uniforms u32 uniform values (or 0/0 when the
   program needs no per-draw uniforms).  A negative `handle` restores the
   fixed-function path (prog_handle defaults to -1 via the request init). */
static inline void g2d_pgl_draw_req_set_shader(g2d_pgl_req_t* req, int32_t handle,
		int32_t unif_shm, uint32_t num_uniforms, uint8_t contig) {
	if(req == NULL)
		return;
	req->prog_handle = handle;
	req->num_uniforms = num_uniforms;
	if(num_uniforms > 0)
		req->unif = g2d_canvas(unif_shm, num_uniforms * 4u, num_uniforms, 1u, contig);
	else
		req->unif = g2d_canvas(0, 0, 0, 0, 0);
}

static inline void g2d_fill_req_init(g2d_fill_req_t* req, g2d_canvas_t dst, g2d_rect_t rect, uint32_t color) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->dst = dst;
	req->rect = rect;
	req->color = color;
}

static inline void g2d_triangle_req_init(g2d_triangle_req_t* req, g2d_canvas_t dst,
		int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3,
		uint32_t color) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->dst = dst;
	req->x1 = x1;
	req->y1 = y1;
	req->x2 = x2;
	req->y2 = y2;
	req->x3 = x3;
	req->y3 = y3;
	req->color = color;
}

static inline void g2d_blit_req_init_ex(g2d_blit_req_t* req,
		g2d_canvas_t dst,
		g2d_canvas_t src,
		g2d_rect_t src_rect,
		g2d_rect_t dst_rect,
		uint8_t alpha,
		int32_t rotate) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->dst = dst;
	req->src = src;
	req->sx = src_rect.x;
	req->sy = src_rect.y;
	req->sw = src_rect.w;
	req->sh = src_rect.h;
	req->dx = dst_rect.x;
	req->dy = dst_rect.y;
	req->dw = dst_rect.w;
	req->dh = dst_rect.h;
	req->alpha = alpha;
	req->rotate = rotate;
}

static inline void g2d_blit_req_init(g2d_blit_req_t* req,
		g2d_canvas_t dst,
		g2d_canvas_t src,
		g2d_rect_t src_rect,
		g2d_rect_t dst_rect,
		uint8_t alpha) {
	g2d_blit_req_init_ex(req, dst, src, src_rect, dst_rect, alpha, G2D_ROTATE_0);
}

static inline void g2d_blit_req_set_rotate(g2d_blit_req_t* req, int32_t rotate) {
	if(req == NULL)
		return;
	req->rotate = rotate;
}

static inline void g2d_rotate_req_init(g2d_rotate_req_t* req, g2d_canvas_t src, g2d_canvas_t dst, int32_t rotate) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->src = src;
	req->dst = dst;
	req->rotate = rotate;
}

static inline void g2d_scale_to_req_init(g2d_scale_to_req_t* req, g2d_canvas_t src, g2d_canvas_t dst) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->src = src;
	req->dst = dst;
}

static inline void g2d_gaussian_blur_req_init(g2d_gaussian_blur_req_t* req,
		g2d_canvas_t dst, g2d_canvas_t tmp, g2d_rect_t rect, int32_t radius) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->dst = dst;
	req->tmp = tmp;
	req->rect = rect;
	req->radius = radius;
}

static inline void g2d_blit_to_phy_req_init(g2d_blit_to_phy_req_t* req,
		g2d_canvas_t src,
		g2d_rect_t src_rect,
		ewokos_addr_t dst_phy, uint32_t dst_size,
		int32_t dst_w, int32_t dst_h, uint32_t pitch,
		g2d_rect_t dst_rect) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->src = src;
	req->sx = src_rect.x;
	req->sy = src_rect.y;
	req->sw = src_rect.w;
	req->sh = src_rect.h;
	req->dst_phy = dst_phy;
	req->dst_size = dst_size;
	req->dst_w = dst_w;
	req->dst_h = dst_h;
	req->pitch = pitch;
	req->dx = dst_rect.x;
	req->dy = dst_rect.y;
	req->dw = dst_rect.w;
	req->dh = dst_rect.h;
}

static inline void g2d_rotate_to_phy_req_init(g2d_rotate_to_phy_req_t* req,
		g2d_canvas_t src,
		g2d_rect_t src_rect,
		ewokos_addr_t dst_phy, uint32_t dst_size,
		int32_t dst_w, int32_t dst_h, uint32_t pitch,
		int32_t dx, int32_t dy, int32_t rotate) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->src = src;
	req->sx = src_rect.x;
	req->sy = src_rect.y;
	req->sw = src_rect.w;
	req->sh = src_rect.h;
	req->dst_phy = dst_phy;
	req->dst_size = dst_size;
	req->dst_w = dst_w;
	req->dst_h = dst_h;
	req->pitch = pitch;
	req->dx = dx;
	req->dy = dy;
	req->rotate = rotate;
}

/* portablegl GPU offload helpers.  g2d_pgl_clear_req_init fills a
   full-frame clear of dst; g2d_pgl_draw_req_init fills a draw of nverts
   primitives from the pos/col vertex canvases into dst, including the depth
   test/write and back-face-cull state (blend stays off; the back end declines
   a blend_en request). */
static inline void g2d_pgl_clear_req_init(g2d_pgl_req_t* req,
		g2d_canvas_t dst, uint32_t fmt, uint32_t stride, uint32_t color,
		uint8_t rb_swap) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->dst = dst;
	req->fmt = fmt;
	req->stride = stride;
	req->color = color;
	req->rb_swap = rb_swap;
	req->prog_handle = -1;   /* fixed-function path (no compiled program) */
}

static inline void g2d_pgl_draw_req_init(g2d_pgl_req_t* req,
		g2d_canvas_t dst, g2d_canvas_t pos, g2d_canvas_t col,
		uint32_t mode, uint32_t nverts, uint32_t fmt, uint32_t stride,
		uint32_t fs_id, uint32_t color, uint8_t rb_swap,
		uint32_t depth_func, uint8_t depth_mask, uint8_t cull_en,
		g2d_canvas_t z, uint32_t z_stride, uint8_t first_frame,
		uint32_t clear_rgba, float clear_z) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->dst = dst;
	req->pos = pos;
	req->col = col;
	req->mode = mode;
	req->nverts = nverts;
	req->fmt = fmt;
	req->stride = stride;
	req->fs_id = fs_id;
	req->color = color;
	req->rb_swap = rb_swap;
	req->depth_func = depth_func;
	req->depth_mask = depth_mask;
	req->cull_en = cull_en;
	req->z = z;
	req->z_stride = z_stride;
	req->first_frame = first_frame;
	req->clear_rgba = clear_rgba;
	req->clear_z = clear_z;
	/* prog_handle defaults to -1 (fixed-function): a memset-zero 0 would
	   alias compiled program slot 0, so the compiled path is opt-in via
	   g2d_pgl_draw_req_set_shader. */
	req->prog_handle = -1;
}

/* Phase D: attach the texture image + per-vertex uv canvases to a draw
   request whose fs_id == G2D_PGL_FS_TEX_MOD.  tex_stride == 0 lets the back
   end derive tex_w * 4.  Call AFTER g2d_pgl_draw_req_init (which memsets the
   whole struct).  A no-op on a NULL req or when fs_id is not TEX_MOD. */
static inline void g2d_pgl_draw_req_set_tex(g2d_pgl_req_t* req,
		g2d_canvas_t tex, uint32_t tex_w, uint32_t tex_h, uint32_t tex_stride,
		g2d_canvas_t uv) {
	if(req == NULL)
		return;
	req->tex = tex;
	req->tex_w = tex_w;
	req->tex_h = tex_h;
	req->tex_stride = tex_stride;
	req->uv = uv;
}

/* Fill a compiled-program upload request.  blob is a contig-shm canvas holding
   a serialized g2d_pgl_program_blob_t header + payload (see the struct
   comment); blob.size must cover both.  Call g2d_pgl_program() to upload. */
static inline void g2d_pgl_program_req_init(g2d_pgl_program_req_t* req,
		g2d_canvas_t blob) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->blob = blob;
}

/* Fill a CL program upload request.  blob is a contig-shm canvas holding a
   serialized g2d_pgl_program_blob_t header + compute-stage payload (see the
   struct comment); blob.size must cover both.  Call g2d_cl_program() to
   upload; it answers the program handle. */
static inline void g2d_cl_program_req_init(g2d_cl_program_req_t* req,
		g2d_canvas_t blob) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->blob = blob;
}

/* Fill a CL dispatch request.  prog_handle is a handle returned by
   g2d_cl_program(); wg_count/wg_size describe the NDRange in work-groups and
   work-items-per-group.  args[]/num_args are the __global buffer SSBO canvases
   in binding order and unif/num_uniforms the packed scalar+__constant uniform
   blob (see g2d_cl_dispatch_req_t).  Call g2d_cl_dispatch() to run it. */
static inline void g2d_cl_dispatch_req_init(g2d_cl_dispatch_req_t* req,
		int32_t prog_handle, uint32_t wgx, uint32_t wgy, uint32_t wgz,
		uint32_t wg_size) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->prog_handle = prog_handle;
	req->wg_count[0] = wgx;
	req->wg_count[1] = wgy;
	req->wg_count[2] = wgz;
	req->wg_size = wg_size;
}

/* Fill a CL program free request. */
static inline void g2d_cl_program_free_req_init(g2d_cl_program_free_req_t* req,
		int32_t prog_handle) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->prog_handle = prog_handle;
}

int has_g2d(void);
int g2d_set_dev(const char* dev);

/* allocate a keyed shm canvas segment (0666 so the driver can attach)
   and map it; a fresh key is used every time because shmget() returns
   an existing keyed segment WITHOUT resizing it. */
int g2d_shm_alloc(uint32_t size, int* shm_id, uint32_t** pixels);
/* Allocate a physically contiguous shared canvas and return its physical
   base.  The physical address is valid for GPU consumers such as VC4. */
int g2d_shm_alloc_phy(uint32_t size, int* shm_id, uint32_t** pixels,
                      ewokos_addr_t* phy);
void g2d_shm_free(uint32_t* pixels);

/* all of these answer with the G2D_* result codes above */
int g2d_fill_rect(const g2d_fill_req_t* req);
int g2d_fill_triangle(const g2d_triangle_req_t* req);
int g2d_blit(const g2d_blit_req_t* req);
int g2d_blit_alpha(const g2d_blit_req_t* req);
int g2d_rotate(const g2d_rotate_req_t* req);
int g2d_scale_to(const g2d_scale_to_req_t* req);
int g2d_blit_to_phy(const g2d_blit_to_phy_req_t* req);
int g2d_rotate_to_phy(const g2d_rotate_to_phy_req_t* req);
int g2d_gaussian_blur(const g2d_gaussian_blur_req_t* req);

/* portablegl GPU (V3D 3D-pipeline) offload.  Both answer with the G2D_*
   result codes: G2D_OK when the GPU ran the op, G2D_ERR_NOT_SUPPORTED
   (sticky) when the platform has no 3D GPU path or the back end declined
   the state/geometry - the caller then runs its own CPU rasterizer and
   stops offloading.  G2D_ERR_FAILED is a transient back-end failure. */
int g2d_pgl_clear(const g2d_pgl_req_t* req);
int g2d_pgl_draw(const g2d_pgl_req_t* req);

/* Upload an on-device-compiled shader program (QPU + uniforms + shader-state
   metadata) serialized in req->blob.  On success returns a NON-NEGATIVE
   program handle that a later g2d_pgl_draw may reference; on failure returns
   a negative G2D_ERR_* code (G2D_ERR_NOT_SUPPORTED when the platform has no
   3D compiler-upload path, so the caller keeps its CPU rasterizer). */
int g2d_pgl_program(const g2d_pgl_program_req_t* req);

/* OpenCL compute offload (raspi5 V3D CSD).  All three answer with the G2D_*
   result codes.  g2d_cl_program() uploads a mesa-compiled compute blob
   (req->blob) and, on success, returns a NON-NEGATIVE program handle a later
   g2d_cl_dispatch() references; on failure it returns a negative G2D_ERR_*
   code (G2D_ERR_NOT_SUPPORTED when the platform has no compute-upload path,
   so libOpenCL keeps its CPU device).  g2d_cl_dispatch() runs one NDRange
   launch; g2d_cl_program_free() releases a handle and its GPU dma. */
int g2d_cl_program(const g2d_cl_program_req_t* req);
int g2d_cl_dispatch(const g2d_cl_dispatch_req_t* req);
int g2d_cl_program_free(const g2d_cl_program_free_req_t* req);

/* query the g2d engine clock rate in Hz (as confirmed by the driver at
   startup; hardware backends pin the GPU to its max rate). returns
   G2D_OK with *hz set, G2D_ERR_NOT_SUPPORTED when the platform has no
   engine clock to report. */
int g2d_get_clock(uint32_t* hz);

/* Query the backend compiler target (V3D 21/42/71; Adreno A690 = 690).
   Returns G2D_OK with *ver set, or G2D_ERR_NOT_SUPPORTED when no compiler
   target is available. The shared app-side compiler selects its backend
   through pglc_version_supported(), without board-specific client code. */
int g2d_get_version(uint32_t* ver);

#ifdef __cplusplus
}
#endif

#endif
