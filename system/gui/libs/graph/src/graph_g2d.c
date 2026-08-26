#include <graph/graph_g2d.h>
#include <g2dclient/g2dclient.h>
#include <string.h>

#ifdef __cplusplus 
extern "C" { 
#endif

inline int graph_has_g2d(void) {
    return has_g2d();
}

/* graph_*_g2d: offload the operation to the /dev/g2d service. the
   device is stateless: every canvas travels inside the request either
   as a keyed shm segment id or as a dma address, the driver attaches
   (shm) or uses the identity dma window (dma) and operates in place.
   only shm-backed graphs (created via graph_new_shm) and dma-backed
   graphs (graph_new_dma) can be processed, zero copy: the device
   writes directly into the graph's own canvas. no cpu fallback: if the
   device path fails, it fails. */

static int g2d_check_graph(const graph_t* g) {
	if(g == NULL || g->buffer == NULL)
		return 0;
	if(g->shm_id <= 0 && !g->dma)
		return 0;
	if(g->w <= 0 || g->h <= 0)
		return 0;
	return 1;
}

static g2d_canvas_t g2d_graph_canvas(const graph_t* g) {
	if(g->dma)
		return g2d_canvas_dma((ewokos_addr_t)(uintptr_t)g->buffer,
				(uint32_t)g->w * (uint32_t)g->h * sizeof(uint32_t),
				(uint32_t)g->w, (uint32_t)g->h);
	return g2d_canvas(g->shm_id,
			(uint32_t)g->w * (uint32_t)g->h * sizeof(uint32_t),
			(uint32_t)g->w, (uint32_t)g->h);
}

void graph_fill_g2d(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	g2d_fill_req_t fill;
	grect_t r;

	if(!g2d_check_graph(g) || w <= 0 || h <= 0)
		return;

	/* same clipping as graph_fill_cpu */
	r.x = x; r.y = y; r.w = w; r.h = h;
	if(!graph_insect(g, &r))
		return;
	if(g->clip.w > 0 && g->clip.h > 0)
		grect_insect(&g->clip, &r);
	if(r.w <= 0 || r.h <= 0)
		return;

	g2d_fill_req_init(&fill, g2d_graph_canvas(g),
			g2d_rect(r.x, r.y, r.w, r.h), color);
	g2d_fill_rect(&fill);
}

static void g2d_do_blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh,
		uint8_t alpha, uint8_t use_alpha) {
	g2d_blit_req_t blit;

	if(!g2d_check_graph(src) || !g2d_check_graph(dst))
		return;
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;

	g2d_blit_req_init_ex(&blit,
			g2d_graph_canvas(dst),
			g2d_graph_canvas(src),
			g2d_rect(sx, sy, sw, sh),
			g2d_rect(dx, dy, dw, dh),
			alpha,
			G2D_ROTATE_0);
	if(use_alpha != 0)
		g2d_blit_alpha_shm(&blit);
	else
		g2d_blit_shm(&blit);
}

void graph_blt_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	g2d_do_blt(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, 0xff, 0);
}

void graph_blt_alpha_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(alpha == 0)
		return;
	g2d_do_blt(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha, 1);
}

/* scales g into dst (dst keeps its own size), nearest neighbor on the
   device */
static void g2d_do_scale(graph_t* g, graph_t* dst, double scale) {
	g2d_scale_to_req_t req;

	(void)scale;
	if(!g2d_check_graph(g) || !g2d_check_graph(dst))
		return;

	g2d_scale_to_req_init(&req, g2d_graph_canvas(g), g2d_graph_canvas(dst));
	g2d_scale_to(&req);
}

void graph_scale_tof_g2d(graph_t* g, graph_t* dst, double scale) {
	if(scale <= 0.0)
		return;
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

	if(!g2d_check_graph(g) || !g2d_check_graph(ret))
		return;

	degree = g2d_rot_degree(rot);
	if(degree == 0)
		return;

	g2d_rotate_req_init(&req, g2d_graph_canvas(g), g2d_graph_canvas(ret), degree);
	g2d_rotate(&req);
}

#ifdef __cplusplus 
}
#endif
