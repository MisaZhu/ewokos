#include <graph/graph_g2d.h>
#include <g2dclient/g2dclient.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>

#ifdef __cplusplus 
extern "C" { 
#endif

inline int graph_has_g2d(void) {
    return has_g2d();
}

/* graph_*_g2d: forward the operation to the /dev/g2d service via
   g2dclient (same request pattern as bin/g2dtest). the device keeps
   its own destination surface and has no pixel readback, so every
   operation is mirrored locally into the graph buffers with the cpu
   paths to keep the caller's pixels correct. source pixels travel
   through a keyed shm segment: an IPC_PRIVATE segment is only visible
   to this process's family, and g2dd is an unrelated process. */

static uint32_t g2d_src_seq = 0;

/* fresh keyed segment per upload: shmget() returns an existing keyed
   segment WITHOUT resizing, so a reused key could be too small for a
   larger source. keyed (non-private) 0666 because g2dd is an unrelated
   process and cannot map IPC_PRIVATE (family-only) segments. the kernel
   frees the segment once both sides have detached. */
static int g2d_src_shm_get(uint32_t size, int* shm_id, uint32_t** pixels) {
	key_t key;
	int id;
	void* addr;

	key = (key_t)(0x47324430u + (((uint32_t)getpid() & 0xffffu) << 16) +
			(g2d_src_seq & 0xffffu));
	g2d_src_seq++;

	id = shmget(key, (int)size, 0666 | IPC_CREAT);
	if(id < 0)
		return -1;
	addr = shmat(id, 0, 0);
	if(addr == (void*)-1)
		return -1;
	*shm_id = id;
	*pixels = (uint32_t*)addr;
	return 0;
}

/* upload the whole source graph into a shm segment the device can
   attach, returns the shm id ready for a blit request */
static int g2d_src_upload(graph_t* src, int* shm_id, uint32_t** pixels) {
	uint32_t size;
	uint32_t* shm;
	int32_t y;

	if(src == NULL || src->buffer == NULL || src->w <= 0 || src->h <= 0)
		return -1;

	size = (uint32_t)src->w * (uint32_t)src->h * sizeof(uint32_t);
	if(g2d_src_shm_get(size, shm_id, &shm) != 0)
		return -1;

	for(y = 0; y < src->h; y++)
		memcpy(shm + (size_t)y * src->w,
				src->buffer + (size_t)y * src->w,
				(size_t)src->w * sizeof(uint32_t));

	*pixels = shm;
	return 0;
}

void graph_fill_g2d(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	g2d_fill_req_t fill;
	grect_t r;

	if(g == NULL || g->buffer == NULL || w <= 0 || h <= 0)
		return;

	/* cpu mirror keeps the local pixels correct */
	graph_fill_cpu(g, x, y, w, h, color);

	/* the device fill is opaque only, skip it for blended colors */
	if(has_g2d() != 0 || color_a(color) != 0xff)
		return;

	r.x = x; r.y = y; r.w = w; r.h = h;
	if(!graph_insect(g, &r))
		return;
	if(g->clip.w > 0 && g->clip.h > 0)
		grect_insect(&g->clip, &r);
	if(r.w <= 0 || r.h <= 0)
		return;

	g2d_fill_req_init(&fill, g2d_rect(r.x, r.y, r.w, r.h), color);
	g2d_fill_rect(&fill);
}

/* upload src and forward the blit to the device; the driver clips the
   dst rect against its surface and rejects out-of-range src rects */
static void g2d_do_blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha, uint8_t use_alpha) {
	g2d_blit_req_t blit;
	uint32_t* pixels;
	int shm_id = -1;

	if(has_g2d() != 0)
		return;
	if(src == NULL || src->buffer == NULL)
		return;
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;
	if(sx < 0 || sy < 0 || sx + sw > src->w || sy + sh > src->h)
		return;

	if(g2d_src_upload(src, &shm_id, &pixels) != 0)
		return;

	g2d_blit_req_init(&blit,
			shm_id,
			(uint32_t)src->w * (uint32_t)src->h * sizeof(uint32_t),
			(uint32_t)src->w,
			(uint32_t)src->h,
			(uint32_t)src->w * sizeof(uint32_t),
			g2d_rect(sx, sy, sw, sh),
			g2d_rect(dx, dy, dw, dh),
			alpha);
	if(use_alpha != 0)
		g2d_blit_alpha_shm(&blit);
	else
		g2d_blit_shm(&blit);
	shmdt(pixels);
}

void graph_blt_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	graph_blt_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
	g2d_do_blt(src, sx, sy, sw, sh, dx, dy, dw, dh, 0xff, 0);
}

void graph_blt_alpha_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	graph_blt_alpha_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
	if(alpha == 0)
		return;
	g2d_do_blt(src, sx, sy, sw, sh, dx, dy, dw, dh, alpha, 1);
}

/* forward "scale the surface to the target size" to the device;
   graph_scale_tof scales g by scale into dst */
static void g2d_do_scale(graph_t* g, graph_t* dst, double scale) {
	g2d_scale_to_req_t req;
	uint32_t tw;
	uint32_t th;

	if(has_g2d() != 0)
		return;
	if(g == NULL || dst == NULL || scale <= 0.0)
		return;

	tw = (uint32_t)((double)g->w * scale);
	th = (uint32_t)((double)g->h * scale);
	if(tw == 0 || th == 0)
		return;
	g2d_scale_to_req_init(&req, tw, th);
	g2d_scale_to(&req);
}

void graph_scale_tof_g2d(graph_t* g, graph_t* dst, double scale) {
	graph_scale_tof_cpu(g, dst, (float)scale);
	g2d_do_scale(g, dst, scale);
}

void graph_scale_tof_fast_g2d(graph_t* g, graph_t* dst, double scale) {
	graph_scale_tof_cpu(g, dst, (float)scale);
	g2d_do_scale(g, dst, scale);
}

/* graph rot values are clockwise 90-degree steps, same as the device */
static int g2d_rot_degree(int rot) {
	switch(rot) {
		case G_ROTATE_90: return G2D_ROTATE_90;
		case G_ROTATE_180: return G2D_ROTATE_180;
		case G_ROTATE_270: return G2D_ROTATE_270;
		default: return 0;
	}
}

void graph_rotate_to_g2d(graph_t* g, graph_t* ret, int rot) {
	g2d_rotate_req_t req;
	int degree;

	graph_rotate_to_cpu(g, ret, rot);

	degree = g2d_rot_degree(rot);
	if(degree == 0 || has_g2d() != 0)
		return;
	g2d_rotate_req_init(&req, degree);
	g2d_rotate(&req);
}

#ifdef __cplusplus 
}
#endif
