#include <graph/graph_g2d.h>
#include <g2dclient/g2dclient.h>
#include <ewoksys/shm.h>
#include <string.h>

#ifdef __cplusplus 
extern "C" { 
#endif

inline int graph_g2d_avaliable(graph_t* g) {
    if(has_g2d() == 0 && g->shm_id > 0)
		return 0;
	return -1;
}

/* graph_*_g2d: offload the operation to the /dev/g2d service. the
   device is stateless: every canvas travels inside the request as a
   keyed shm segment id, the driver attaches and operates in place.
   only shm-backed graphs (created via graph_new_shm) can be processed,
   zero copy: the device writes directly into the graph's own canvas.

   A -1 from here is NOT terminal: every dispatcher (graph_blt,
   graph_fill_rect, graph_blt_alpha, graph_rotate_to, graph_scale_tof)
   treats it as "run the cpu/arch pass instead", in the calling process.
   So a canvas that lost its contig backing degrades silently into a
   full-frame cpu copy with no error and no log anywhere - the counters
   below are the only way to see it happen. */

static uint32_t _g2d_reject_num = 0;
static uint32_t _g2d_reject_noncontig = 0;
static uint32_t _g2d_reject_small = 0;
static uint64_t _g2d_reject_px = 0;

void graph_g2d_reject_stats(uint32_t* num, uint32_t* noncontig,
		uint32_t* small, uint64_t* pixels) {
	if(num != NULL)
		*num = _g2d_reject_num;
	if(noncontig != NULL)
		*noncontig = _g2d_reject_noncontig;
	if(small != NULL)
		*small = _g2d_reject_small;
	if(pixels != NULL)
		*pixels = _g2d_reject_px;
}

/* record a turned-down request and split the reason: a shm-backed canvas
   without contig backing is the interesting one (graph_new_shm falls back
   to a plain segment when shmget(IPC_CONTIG) cannot be satisfied), a
   below-G2D_MIN_SIZE canvas is expected and cheap. */
static int g2d_reject(const graph_t* a, const graph_t* b, int32_t w, int32_t h) {
	_g2d_reject_num++;
	if(w > 0 && h > 0)
		_g2d_reject_px += (uint64_t)w * (uint64_t)h;
	if((a != NULL && a->shm_id > 0 && !a->shm_contig) ||
			(b != NULL && b->shm_id > 0 && !b->shm_contig))
		_g2d_reject_noncontig++;
	else
		_g2d_reject_small++;
	return -1;
}

static int g2d_check_graph(const graph_t* g) {
	if(g == NULL || g->buffer == NULL)
		return 0;
	if(g->shm_id <= 0 || !g->shm_contig)
		return 0;
	/*
	if(g->w <= 0 || g->h <= 0)
		return 0;
	*/

	if((g->w * g->h) < G2D_MIN_SIZE)
		return 0;
	return 1;
}

static g2d_canvas_t g2d_graph_canvas(const graph_t* g) {
	g2d_canvas_t canvas = g2d_canvas(g->shm_id,
			(uint32_t)g->w * (uint32_t)g->h * sizeof(uint32_t),
			(uint32_t)g->w, (uint32_t)g->h,
			g->shm_contig ? 1 : 0);
	/* contig shm canvases travel with their resolved physical base so
	   the driver's hardware 2d path can work on physical addresses
	   directly (the shm window is mapped at the same vaddr in every
	   process, so the client-side translation is valid driver-side) */
	if(g->shm_contig)
		canvas.phy = shm_contig_phy_addr(g->shm_id, (ewokos_addr_t)g->buffer);
	return canvas;
}

int graph_fill_g2d(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	g2d_fill_req_t fill;
	grect_t r;

	if(!g2d_check_graph(g) || (w * h < G2D_MIN_SIZE))
		return g2d_reject(g, NULL, w, h);

	/* same clipping as graph_fill_cpu */
	r.x = x; r.y = y; r.w = w; r.h = h;
	if(!graph_insect(g, &r))
		return -1;
	if(g->clip.w > 0 && g->clip.h > 0)
		grect_insect(&g->clip, &r);
	if(r.w <= 0 || r.h <= 0)
		return -1;

	g2d_fill_req_init(&fill, g2d_graph_canvas(g),
			g2d_rect(r.x, r.y, r.w, r.h), color);
	return g2d_fill_rect(&fill);
}

static int g2d_do_blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh,
		uint8_t alpha, uint8_t use_alpha) {
	g2d_blit_req_t blit;

	if(!g2d_check_graph(src) || !g2d_check_graph(dst))
		return -1;
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return -1;

	g2d_blit_req_init_ex(&blit,
			g2d_graph_canvas(dst),
			g2d_graph_canvas(src),
			g2d_rect(sx, sy, sw, sh),
			g2d_rect(dx, dy, dw, dh),
			alpha,
			G2D_ROTATE_0);
	if(use_alpha != 0)
		return g2d_blit_alpha(&blit);
	return g2d_blit(&blit);
}

int graph_blt_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	if(!g2d_check_graph(src) || !g2d_check_graph(dst))
		return g2d_reject(src, dst, dw, dh);
	return g2d_do_blt(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, 0xff, 0);
}

int graph_blt_alpha_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(alpha == 0)
		return 0;

	if(!g2d_check_graph(src) || !g2d_check_graph(dst))
		return g2d_reject(src, dst, dw, dh);
	return g2d_do_blt(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha, 1);
}

/* scales g into dst (dst keeps its own size), nearest neighbor on the
   device */
static int g2d_do_scale(graph_t* g, graph_t* dst, double scale) {
	g2d_scale_to_req_t req;
	(void)scale;

	g2d_scale_to_req_init(&req, g2d_graph_canvas(g), g2d_graph_canvas(dst));
	return g2d_scale_to(&req);
}

int graph_scale_tof_g2d(graph_t* g, graph_t* dst, double scale) {
	if(scale <= 0.0)
		return 0;
	if(!g2d_check_graph(g) && !g2d_check_graph(dst))
		return -1;
	return g2d_do_scale(g, dst, scale);
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

int graph_rotate_to_g2d(graph_t* g, graph_t* ret, int rot) {
	g2d_rotate_req_t req;
	int degree;

	if(!g2d_check_graph(g) || !g2d_check_graph(ret))
		return -1;

	degree = g2d_rot_degree(rot);
	if(degree == 0)
		return 0;

	g2d_rotate_req_init(&req, g2d_graph_canvas(g), g2d_graph_canvas(ret), degree);
	return g2d_rotate(&req);
}

#ifdef __cplusplus 
}
#endif
