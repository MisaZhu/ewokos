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
	G2D_DEV_CNTL_SCALE_TO
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
   of its own, every request carries the canvases it works on. */
typedef struct {
	int32_t shm_id;
	uint32_t size;  /* segment size in bytes, must be >= w*h*4 */
	uint32_t w;
	uint32_t h;
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

static inline g2d_canvas_t g2d_canvas(int32_t shm_id, uint32_t size, uint32_t w, uint32_t h) {
	g2d_canvas_t canvas;
	canvas.shm_id = shm_id;
	canvas.size = size;
	canvas.w = w;
	canvas.h = h;
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

int has_g2d(void);
int g2d_set_dev(const char* dev);

/* allocate a keyed shm canvas segment (0666 so the driver can attach)
   and map it; a fresh key is used every time because shmget() returns
   an existing keyed segment WITHOUT resizing it. */
int g2d_shm_alloc(uint32_t size, int* shm_id, uint32_t** pixels);
void g2d_shm_free(uint32_t* pixels);

int g2d_fill_rect(const g2d_fill_req_t* req);
int g2d_blit_shm(const g2d_blit_req_t* req);
int g2d_blit_alpha_shm(const g2d_blit_req_t* req);
int g2d_rotate(const g2d_rotate_req_t* req);
int g2d_scale_to(const g2d_scale_to_req_t* req);

#ifdef __cplusplus
}
#endif

#endif
