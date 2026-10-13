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
	G2D_DEV_CNTL_PGL_DRAW
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
/* maximum vertices per draw dispatch (the back end declines more) */
#define G2D_PGL_MAX_VERTS 4096u

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

/* separable Gaussian blur of a sub-rect of the dst canvas, in place (the
   default blur algorithm). rect is clamped into dst by the driver; a rect
   covering the whole canvas blurs everything. the driver is GPU-only
   (no CPU fallback, same as fill/blit/rotate): the back end runs a
   whole-canvas blur at a width that is a multiple of 16 with any
   radius >= 1 (radius 1..4 dispatch their own separable kernel pair
   with fixed per-radius Q16 weights, sigma = radius/2, so the request
   carries no coefficients; larger radii compose the same stages), and
   needs the tmp scratch canvas: same geometry class as dst, size >=
   dst.w*dst.h*4, GPU-visible like dst (the driver only passes it
   through and never reads it). a partial rect or a non-aligned width
   is not runnable - G2D_ERR_FAILED, nothing submitted (run your own
   software blur); G2D_ERR_FAILED also covers a bad canvas, an empty
   rect, a non-positive radius or a failed dispatch (transient). */
typedef struct {
	g2d_canvas_t dst;
	g2d_canvas_t tmp;
	g2d_rect_t rect;
	int32_t radius;
} g2d_gaussian_blur_req_t;

/* Extended per-dispatch fixed-function state (Phase B2c): the scissor test
   window, polygon offset and colour write mask.  Carried as a sub-struct so
   bsp_g2d_pgl_draw grows by ONE opaque pointer instead of eight scalars: the
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
} g2d_pgl_ext_t;

/* Phase D texture state passed to bsp_g2d_pgl_draw as an opaque pointer
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
} g2d_pgl_req_t;

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

/* query the g2d engine clock rate in Hz (as confirmed by the driver at
   startup; hardware backends pin the GPU to its max rate). returns
   G2D_OK with *hz set, G2D_ERR_NOT_SUPPORTED when the platform has no
   engine clock to report. */
int g2d_get_clock(uint32_t* hz);

#ifdef __cplusplus
}
#endif

#endif
