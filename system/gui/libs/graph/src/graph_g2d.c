#include <graph/graph_g2d.h>
#include <g2dclient/g2dclient.h>
#include <ewoksys/shm.h>
#include <ewoksys/klog.h>
#include <ewoksys/kernel_tic.h>
#include <stdio.h>
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

   Any non-zero from here is NOT terminal: every dispatcher (graph_blt,
   graph_fill_rect, graph_blt_alpha, graph_rotate_to, graph_scale_tof)
   treats it as "run the cpu/arch pass instead", in the calling process.
   So a canvas that lost its contig backing degrades silently into a
   full-frame cpu copy with no error and no log anywhere - the counters
   below are the only way to see it happen.

   G2D_ERR_NOT_SUPPORTED is the one answer that gets remembered: the
   driver is saying the op itself is not there, so it is marked per op
   below and that op is never offloaded again. */

/* sticky per-op capability bits: a declined capability is a property of
   the running driver, not of the request - asking again can never change
   the answer. once an op is marked, its entry point fails right away and
   the dispatcher runs the cpu/arch pass directly, with no ipc round trip
   per frame for an answer that cannot change. the mark is per op: one
   missing capability never switches the others off. */
enum {
	G2D_CAP_FILL = 0,
	G2D_CAP_BLT,
	G2D_CAP_BLIT_ALPHA,
	G2D_CAP_ROTATE,
	G2D_CAP_SCALE_TO,
	G2D_CAP_GAUSSIAN_BLUR
};

static uint32_t _g2d_unsupported = 0;

static int g2d_op_supported(int cap) {
	return (_g2d_unsupported & (1u << cap)) == 0;
}

/* hand the g2dclient result to the dispatcher, marking the op when the
   driver reported the capability missing */
static int g2d_op_result(int cap, int ret) {
	if(ret == G2D_ERR_NOT_SUPPORTED)
		_g2d_unsupported |= (1u << cap);
	return ret;
}

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
/* gaussian-blur routing counters: the blur has fallback reasons the
   generic reject stats cannot see (per-call scratch allocation, the
   driver refusing an otherwise-eligible request), and xwm blurs whole
   windows every frame - when one of those quietly drops to the cpu pass
   the compositor's cpu time spikes with no error anywhere. All pointers
   optional. */
static uint32_t _g2d_blur_gpu = 0;      /* dispatched to /dev/g2d */
static uint32_t _g2d_blur_fb_notsup = 0;  /* op marked unsupported */
static uint32_t _g2d_blur_fb_canvas = 0;  /* non-shm/contig/too-small */
static uint32_t _g2d_blur_fb_rect = 0;    /* partial rect (by design) */
static uint32_t _g2d_blur_fb_tmp = 0;     /* scratch alloc failed */
static uint32_t _g2d_blur_fb_drv = 0;     /* driver answered non-zero */

void graph_g2d_blur_stats(uint32_t* gpu, uint32_t* fb_notsup,
		uint32_t* fb_canvas, uint32_t* fb_rect, uint32_t* fb_tmp,
		uint32_t* fb_drv) {
	if(gpu != NULL) *gpu = _g2d_blur_gpu;
	if(fb_notsup != NULL) *fb_notsup = _g2d_blur_fb_notsup;
	if(fb_canvas != NULL) *fb_canvas = _g2d_blur_fb_canvas;
	if(fb_rect != NULL) *fb_rect = _g2d_blur_fb_rect;
	if(fb_tmp != NULL) *fb_tmp = _g2d_blur_fb_tmp;
	if(fb_drv != NULL) *fb_drv = _g2d_blur_fb_drv;
}

/* one line per second per reason while the condition persists: enough
   to see a routing break in /dev/log, quiet when routing works */
static void blur_fallback_klog(const char* why, uint32_t n) {
	static uint64_t _last_ms[4];
	static uint32_t _last_cnt[4];
	int slot = (why[0] >> 1) & 3;
	uint64_t now = kernel_tic_ms(0);

	if(_last_ms[slot] != 0 && now - _last_ms[slot] >= 1000) {
		klog("graph: blur fell back to cpu (%s) x%u in the last %ums\n",
				why, n - _last_cnt[slot],
				(unsigned)(now - _last_ms[slot]));
	}
	if(_last_ms[slot] == 0 || now - _last_ms[slot] >= 1000) {
		_last_ms[slot] = now;
		_last_cnt[slot] = n;
	}
}

static int g2d_reject(const graph_t* a, const graph_t* b, int32_t w, int32_t h) {
	_g2d_reject_num++;
	if(w > 0 && h > 0)
		_g2d_reject_px += (uint64_t)w * (uint64_t)h;
	if((a != NULL && a->shm_id > 0 && !a->shm_contig) ||
			(b != NULL && b->shm_id > 0 && !b->shm_contig))
		_g2d_reject_noncontig++;
	else
		_g2d_reject_small++;
	return G2D_ERR_FAILED;
}

/* w/h describe the rect the operation actually touches: offloading is
   only worth the ipc round trip when that rect carries enough pixels, a
   canvas-only check would still send every thin frame band to the device.
   a 0 w or h means "no rect of my own" (rotate, scale) and falls back to
   the full canvas dimensions. */
static int g2d_check_graph(const graph_t* g, int32_t w, int32_t h) {
	if(g == NULL || g->buffer == NULL)
		return 0;
	if(g->shm_id <= 0 || !g->shm_contig)
		return 0;
	if(w <= 0)
		w = g->w;
	if(h <= 0)
		h = g->h;
	if(w <= 0 || h <= 0)
		return 0;

	if((w * h) < G2D_MIN_SIZE)
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

	if(!g2d_op_supported(G2D_CAP_FILL))
		return G2D_ERR_NOT_SUPPORTED;

	if(!g2d_check_graph(g, w, h))
		return g2d_reject(g, NULL, w, h);

	/* same clipping as graph_fill_cpu */
	r.x = x; r.y = y; r.w = w; r.h = h;
	if(!graph_insect(g, &r))
		return G2D_ERR_FAILED;
	if(g->clip.w > 0 && g->clip.h > 0)
		grect_insect(&g->clip, &r);
	if(r.w <= 0 || r.h <= 0)
		return G2D_ERR_FAILED;

	g2d_fill_req_init(&fill, g2d_graph_canvas(g),
			g2d_rect(r.x, r.y, r.w, r.h), color);
	return g2d_op_result(G2D_CAP_FILL, g2d_fill_rect(&fill));
}

static int g2d_do_blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh,
		uint8_t alpha, uint8_t use_alpha) {
	g2d_blit_req_t blit;
	int cap = (use_alpha != 0) ? G2D_CAP_BLIT_ALPHA : G2D_CAP_BLT;

	if(!g2d_op_supported(cap))
		return G2D_ERR_NOT_SUPPORTED;
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return G2D_ERR_FAILED;

	g2d_blit_req_init_ex(&blit,
			g2d_graph_canvas(dst),
			g2d_graph_canvas(src),
			g2d_rect(sx, sy, sw, sh),
			g2d_rect(dx, dy, dw, dh),
			alpha,
			G2D_ROTATE_0);
	if(use_alpha != 0)
		return g2d_op_result(cap, g2d_blit_alpha(&blit));
	return g2d_op_result(cap, g2d_blit(&blit));
}

int graph_blt_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	if(!g2d_op_supported(G2D_CAP_BLT))
		return G2D_ERR_NOT_SUPPORTED;
	if(!g2d_check_graph(src, sw, sh) || !g2d_check_graph(dst, dw, dh))
		return g2d_reject(src, dst, dw, dh);
	return g2d_do_blt(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, 0xff, 0);
}

int graph_blt_alpha_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(alpha == 0)
		return 0;

	if(!g2d_op_supported(G2D_CAP_BLIT_ALPHA))
		return G2D_ERR_NOT_SUPPORTED;
	if(!g2d_check_graph(src, sw, sh) || !g2d_check_graph(dst, dw, dh))
		return g2d_reject(src, dst, dw, dh);
	return g2d_do_blt(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha, 1);
}

/* scales g into dst (dst keeps its own size), nearest neighbor on the
   device */
static int g2d_do_scale(graph_t* g, graph_t* dst, double scale) {
	g2d_scale_to_req_t req;
	(void)scale;

	g2d_scale_to_req_init(&req, g2d_graph_canvas(g), g2d_graph_canvas(dst));
	return g2d_op_result(G2D_CAP_SCALE_TO, g2d_scale_to(&req));
}

int graph_scale_tof_g2d(graph_t* g, graph_t* dst, double scale) {
	if(scale <= 0.0)
		return 0;
	if(!g2d_op_supported(G2D_CAP_SCALE_TO))
		return G2D_ERR_NOT_SUPPORTED;
	if(!g2d_check_graph(g, 0, 0) && !g2d_check_graph(dst, 0, 0))
		return G2D_ERR_FAILED;
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

	if(!g2d_op_supported(G2D_CAP_ROTATE))
		return G2D_ERR_NOT_SUPPORTED;

	if(!g2d_check_graph(g, 0, 0) || !g2d_check_graph(ret, 0, 0))
		return G2D_ERR_FAILED;

	degree = g2d_rot_degree(rot);
	if(degree == 0)
		return 0;

	g2d_rotate_req_init(&req, g2d_graph_canvas(g), g2d_graph_canvas(ret), degree);
	return g2d_op_result(G2D_CAP_ROTATE, g2d_rotate(&req));
}

/* in-place gaussian blur of a sub-rect on the device. the caller imposes no
   geometry restriction: the rect (x,y,w,h) and the GPU-vs-software choice
   are all resolved by g2dd (a partial rect is refused by the back end).
   only the radius is filtered here - a radius other than 2/4 can never reach the
   hardware back end, and blurring it in-process (the arch/cpu pass the
   dispatcher falls back to) is cheaper than a round trip to the daemon for
   the very same software engine, so those are simply not offloaded.

   the hardware (GPU) back end needs a scratch canvas (tmp) and only runs a
   whole-canvas, 16-aligned, radius-2/4 blur; that is the only case tmp is
   allocated for. any other rect goes to the driver's software engine, which
   ignores tmp, so a zeroed tmp canvas is sent and no shm is allocated. tmp
   is freed right after the call: caching it would pin a full-canvas shm
   segment for the life of the process with no owner to release it. */
/* the blur scratch is an inter-pass working surface the driver never
   reads after the dispatch: cache one per process and grow it on demand
   instead of allocating and freeing a w*h*4 contig segment per frame -
   the per-call shm round trip is pure cpu overhead in the compositor's
   frame path and fragmentation-prone on the contig pool. */
static int _blur_tmp_shm_id = -1;
static uint32_t* _blur_tmp_pixels = NULL;
static uint32_t _blur_tmp_size = 0;
static ewokos_addr_t _blur_tmp_phy = 0;

static int blur_tmp_get(uint32_t size, int* shm_id, uint32_t** pixels,
		ewokos_addr_t* phy) {
	if(_blur_tmp_pixels != NULL && _blur_tmp_size >= size) {
		*shm_id = _blur_tmp_shm_id;
		*pixels = _blur_tmp_pixels;
		*phy = _blur_tmp_phy;
		return 0;
	}
	if(_blur_tmp_pixels != NULL) {
		g2d_shm_free(_blur_tmp_pixels);
		_blur_tmp_pixels = NULL;
		_blur_tmp_shm_id = -1;
		_blur_tmp_size = 0;
		_blur_tmp_phy = 0;
	}
	if(g2d_shm_alloc_phy(size, &_blur_tmp_shm_id, &_blur_tmp_pixels,
			&_blur_tmp_phy) != 0)
		return -1;
	_blur_tmp_size = size;
	*shm_id = _blur_tmp_shm_id;
	*pixels = _blur_tmp_pixels;
	*phy = _blur_tmp_phy;
	return 0;
}

int graph_gaussian_blur_g2d(graph_t* g, int x, int y, int w, int h, int r) {
	g2d_gaussian_blur_req_t req;
	g2d_canvas_t tmp;
	int tmp_shm_id = -1;
	uint32_t* tmp_pixels = NULL;
	ewokos_addr_t tmp_phy = 0;
	int gpu_path;
	int ret;
	uint64_t t0, t1;

	if(!g2d_op_supported(G2D_CAP_GAUSSIAN_BLUR)) {
		_g2d_blur_fb_notsup++;
		blur_fallback_klog("op unsupported", _g2d_blur_fb_notsup);
		return G2D_ERR_NOT_SUPPORTED;
	}

	if(r <= 0)
		return G2D_ERR_FAILED;

	if(!g2d_check_graph(g, w, h)) {
		_g2d_blur_fb_canvas++;
		blur_fallback_klog("canvas not shm/contig",
				   _g2d_blur_fb_canvas);
		return g2d_reject(g, NULL, w, h);
	}

	/* the hardware back end runs any rect of the canvas at any width
	   (the kernels tail-mask the final partial 16-px group of each
	   row).  an out-of-canvas rect is refused locally - the driver
	   would answer -1 with nothing submitted. */
	gpu_path = (x >= 0 && y >= 0 && w > 0 && h > 0 &&
			x <= g->w - w && y <= g->h - h);
	if(!gpu_path) {
		_g2d_blur_fb_rect++;
		return G2D_ERR_FAILED;
	}

	memset(&tmp, 0, sizeof(tmp));
	/* the scratch is a pitch-strided (h - 1)-row region of the canvas
	   pitch plus one rect row (see bsp_g2d.h) */
	if(blur_tmp_get((uint32_t)(h - 1) * (uint32_t)g->w * 4u +
				(uint32_t)w * 4u,
			&tmp_shm_id, &tmp_pixels, &tmp_phy) != 0) {
		_g2d_blur_fb_tmp++;
		blur_fallback_klog("scratch alloc failed", _g2d_blur_fb_tmp);
		return G2D_ERR_FAILED;
	}
	/* describe the scratch as its LOGICAL rw x rh geometry: the attach
	   validation checks size >= w*h*4, and (h - 1) rows of the CANVAS
	   pitch plus one rect row can be smaller than canvas_w * rect_h *
	   4 whenever the rect is narrower than the canvas (the shadow
	   strips) - those requests were rejected at attach as undersized
	   even though the scratch provably holds every row the kernels
	   read or write */
	tmp = g2d_canvas(tmp_shm_id, _blur_tmp_size, (uint32_t)w,
			(uint32_t)h, 1);
	tmp.phy = tmp_phy;

	g2d_gaussian_blur_req_init(&req, g2d_graph_canvas(g), tmp,
			g2d_rect(x, y, w, h), r);
	t0 = kernel_tic_ms(0);
	ret = g2d_op_result(G2D_CAP_GAUSSIAN_BLUR, g2d_gaussian_blur(&req));
	t1 = kernel_tic_ms(0);
	if(ret == 0) {
		_g2d_blur_gpu++;
	}
	else {
		char why[128];

		_g2d_blur_fb_drv++;
		/* the elapsed round-trip separates an ipc queue timeout
		   (g2dd busy - the request may never have been processed)
		   from a genuine refusal (fast answer) */
		snprintf(why, sizeof(why),
				"blur ret=%d after %ums (canvas %dx%d, rect "
				"%d,%d %dx%d, r=%d)",
				ret, (unsigned)(t1 - t0),
				g->w, g->h, x, y, w, h, r);
		blur_fallback_klog(why, _g2d_blur_fb_drv);
	}
	return ret;
}

#ifdef __cplusplus 
}
#endif
