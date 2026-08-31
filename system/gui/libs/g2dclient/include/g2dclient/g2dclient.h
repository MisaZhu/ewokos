#ifndef G2D_G2D_CLIENT_H
#define G2D_G2D_CLIENT_H

#include <stdint.h>
#include <string.h>
#include <ewoksys/fsinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	G2D_DEV_CNTL_FILL_RECT = 0,
	G2D_DEV_CNTL_BLIT,
	G2D_DEV_CNTL_BLIT_ALPHA,
	G2D_DEV_CNTL_ROTATE,
	G2D_DEV_CNTL_SCALE_TO,
	G2D_DEV_CNTL_BLIT_TO_PHY
};

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
   that work on physical addresses; phy then carries the resolved
   physical base of the segment (shm_contig_phy_addr on the client
   side), 0 when unknown.
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

int has_g2d(void);
int g2d_set_dev(const char* dev);

/* allocate a keyed shm canvas segment (0666 so the driver can attach)
   and map it; a fresh key is used every time because shmget() returns
   an existing keyed segment WITHOUT resizing it. */
int g2d_shm_alloc(uint32_t size, int* shm_id, uint32_t** pixels);
void g2d_shm_free(uint32_t* pixels);

int g2d_fill_rect(const g2d_fill_req_t* req);
int g2d_blit(const g2d_blit_req_t* req);
int g2d_blit_alpha(const g2d_blit_req_t* req);
int g2d_rotate(const g2d_rotate_req_t* req);
int g2d_scale_to(const g2d_scale_to_req_t* req);
int g2d_blit_to_phy(const g2d_blit_to_phy_req_t* req);

#ifdef __cplusplus
}
#endif

#endif
