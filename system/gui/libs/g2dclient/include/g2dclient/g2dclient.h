#ifndef G2D_G2D_CLIENT_H
#define G2D_G2D_CLIENT_H

#include <stdint.h>
#include <string.h>
#include <ewoksys/fsinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	G2D_DEV_CNTL_GET_INFO = 0,
	G2D_DEV_CNTL_CLEAR,
	G2D_DEV_CNTL_FILL_RECT,
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

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t format;
	uint32_t backend;
} g2d_info_t;

typedef struct {
	g2d_rect_t rect;
	uint32_t color;
} g2d_fill_req_t;

/* rotates the destination surface clockwise by degree (any angle,
   normalized to [0, 360) by the driver). 90/270 swap the surface
   dimensions, other angles grow to the rotated bounding box. */
typedef struct {
	int32_t rotate;
} g2d_rotate_req_t;

/* scales the destination surface to width x height. */
typedef struct {
	uint32_t width;
	uint32_t height;
} g2d_scale_to_req_t;

typedef struct {
	int32_t x;
	int32_t y;
} g2d_pixel_req_t;

typedef struct {
	uint32_t src_w;
	uint32_t src_h;
	uint32_t src_stride;
	uint32_t src_format;
	int32_t src_shm_id;
	uint32_t src_size;
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

static inline void g2d_fill_req_init(g2d_fill_req_t* req, g2d_rect_t rect, uint32_t color) {
	if(req == NULL)
		return;
	req->rect = rect;
	req->color = color;
}

static inline void g2d_blit_req_init_ex(g2d_blit_req_t* req,
		int32_t src_shm_id,
		uint32_t src_size,
		uint32_t src_w,
		uint32_t src_h,
		uint32_t src_stride,
		g2d_rect_t src_rect,
		g2d_rect_t dst_rect,
		uint8_t alpha,
		int32_t rotate) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->src_w = src_w;
	req->src_h = src_h;
	req->src_stride = src_stride;
	req->src_format = G2D_FMT_ARGB8888;
	req->src_shm_id = src_shm_id;
	req->src_size = src_size;
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
		int32_t src_shm_id,
		uint32_t src_size,
		uint32_t src_w,
		uint32_t src_h,
		uint32_t src_stride,
		g2d_rect_t src_rect,
		g2d_rect_t dst_rect,
		uint8_t alpha) {
	g2d_blit_req_init_ex(req,
			src_shm_id,
			src_size,
			src_w,
			src_h,
			src_stride,
			src_rect,
			dst_rect,
			alpha,
			G2D_ROTATE_0);
}

static inline void g2d_blit_req_set_rotate(g2d_blit_req_t* req, int32_t rotate) {
	if(req == NULL)
		return;
	req->rotate = rotate;
}

static inline void g2d_rotate_req_init(g2d_rotate_req_t* req, int32_t rotate) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->rotate = rotate;
}

static inline void g2d_scale_to_req_init(g2d_scale_to_req_t* req, uint32_t width, uint32_t height) {
	if(req == NULL)
		return;
	memset(req, 0, sizeof(*req));
	req->width = width;
	req->height = height;
}

int has_g2d(void);
int g2d_set_dev(const char* dev);
int g2d_info(g2d_info_t* info);
int g2d_clear(uint32_t color);
int g2d_fill_rect(const g2d_fill_req_t* req);
int g2d_blit_shm(const g2d_blit_req_t* req);
int g2d_blit_alpha_shm(const g2d_blit_req_t* req);
int g2d_rotate(const g2d_rotate_req_t* req);
int g2d_scale_to(const g2d_scale_to_req_t* req);

#ifdef __cplusplus
}
#endif

#endif
