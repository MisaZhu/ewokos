#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <ewoksys/proto.h>
#include <ewoksys/vdevice.h>
#include <graph/graph.h>
#include <tinyjson/tinyjson.h>
#include <g2d/g2d.h>

typedef struct {
	uint32_t width;
	uint32_t height;
	graph_t* graph;
} g2d_surface_t;

typedef struct {
	graph_t view;
	graph_t* owned_graph;
	void* shm_ptr;
} g2d_import_t;

typedef struct {
	g2d_surface_t dst;
	uint32_t clear_color;
	g2d_stats_t stats;
} g2d_state_t;

static int32_t g2d_surface_init(g2d_surface_t* surface, uint32_t width, uint32_t height) {
	if (surface == NULL || width == 0 || height == 0)
		return -1;

	memset(surface, 0, sizeof(*surface));
	surface->graph = graph_new(NULL, width, height);
	if (surface->graph == NULL || surface->graph->buffer == NULL) {
		if (surface->graph != NULL) {
			graph_free(surface->graph);
			surface->graph = NULL;
		}
		return -1;
	}

	surface->width = width;
	surface->height = height;
	return 0;
}

static void g2d_surface_release(g2d_surface_t* surface) {
	if (surface == NULL)
		return;
	if (surface->graph != NULL) {
		graph_free(surface->graph);
		surface->graph = NULL;
	}
	surface->width = 0;
	surface->height = 0;
}

static int32_t g2d_surface_clear(g2d_surface_t* surface, uint32_t color) {
	if (surface == NULL || surface->graph == NULL)
		return -1;
	graph_clear(surface->graph, color);
	return 0;
}

static int32_t g2d_surface_fill_rect(g2d_surface_t* surface, const g2d_fill_req_t* req) {
	if (surface == NULL || surface->graph == NULL || req == NULL)
		return -1;
	graph_fill_rect(surface->graph,
			req->rect.x, req->rect.y,
			req->rect.w, req->rect.h,
			req->color);
	return 0;
}

static int32_t g2d_blit_rotate_valid(uint8_t rotate) {
	switch (rotate) {
	case G2D_ROTATE_0:
	case G2D_ROTATE_90:
	case G2D_ROTATE_180:
	case G2D_ROTATE_270:
		return 1;
	default:
		return 0;
	}
}

static graph_t* g2d_surface_crop_source(graph_t* src, const g2d_blit_req_t* req) {
	graph_t* cropped;

	if (src == NULL || req == NULL)
		return NULL;
	if (req->sx < 0 || req->sy < 0 || req->sw <= 0 || req->sh <= 0)
		return NULL;
	if (req->sx + req->sw > src->w || req->sy + req->sh > src->h)
		return NULL;

	cropped = graph_new(NULL, req->sw, req->sh);
	if (cropped == NULL || cropped->buffer == NULL) {
		if (cropped != NULL)
			graph_free(cropped);
		return NULL;
	}

	graph_blt(src,
			req->sx, req->sy, req->sw, req->sh,
			cropped,
			0, 0, req->sw, req->sh);
	return cropped;
}

static graph_t* g2d_surface_prepare_source(graph_t* src, const g2d_blit_req_t* req) {
	graph_t* cropped;
	graph_t* rotated;

	if (src == NULL || req == NULL)
		return NULL;
	if (!g2d_blit_rotate_valid(req->rotate))
		return NULL;

	cropped = g2d_surface_crop_source(src, req);
	if (cropped == NULL)
		return NULL;
	if (req->rotate == G2D_ROTATE_0)
		return cropped;

	rotated = graph_rotate(cropped, req->rotate);
	graph_free(cropped);
	return rotated;
}

static int32_t g2d_surface_render_prepared(g2d_surface_t* dst, graph_t* prepared, const g2d_blit_req_t* req, uint8_t use_alpha) {
	if (dst == NULL || dst->graph == NULL || prepared == NULL || req == NULL)
		return -1;
	if (req->dw <= 0 || req->dh <= 0)
		return -1;

	if (use_alpha != 0) {
		if (prepared->w == req->dw && prepared->h == req->dh) {
			graph_blt_alpha(prepared,
					0, 0, prepared->w, prepared->h,
					dst->graph,
					req->dx, req->dy, req->dw, req->dh,
					req->alpha);
		}
		else {
			graph_blt_fit_alpha(prepared,
					0, 0, prepared->w, prepared->h,
					dst->graph,
					req->dx, req->dy, req->dw, req->dh,
					req->alpha);
		}
	}
	else {
		if (prepared->w == req->dw && prepared->h == req->dh) {
			graph_blt(prepared,
					0, 0, prepared->w, prepared->h,
					dst->graph,
					req->dx, req->dy, req->dw, req->dh);
		}
		else {
			graph_blt_fit(prepared,
					0, 0, prepared->w, prepared->h,
					dst->graph,
					req->dx, req->dy, req->dw, req->dh);
		}
	}
	return 0;
}

static int32_t g2d_surface_blit(g2d_surface_t* dst, graph_t* src, const g2d_blit_req_t* req, uint8_t use_alpha) {
	graph_t* prepared;
	int32_t ret;

	if (dst == NULL || dst->graph == NULL || src == NULL || req == NULL)
		return -1;
	prepared = g2d_surface_prepare_source(src, req);
	if (prepared == NULL)
		return -1;

	ret = g2d_surface_render_prepared(dst, prepared, req, use_alpha);
	graph_free(prepared);
	return ret;
}

static int32_t g2d_surface_get_pixel(g2d_surface_t* surface, int32_t x, int32_t y, uint32_t* pixel) {
	if (surface == NULL || surface->graph == NULL || pixel == NULL)
		return -1;
	if (x < 0 || y < 0 || x >= surface->graph->w || y >= surface->graph->h)
		return -1;

	*pixel = graph_get_pixel(surface->graph, x, y);
	return 0;
}

static void g2d_import_init(g2d_import_t* import) {
	if (import == NULL)
		return;
	memset(import, 0, sizeof(*import));
}

static void g2d_import_release(g2d_import_t* import) {
	if (import == NULL)
		return;
	if (import->owned_graph != NULL)
		graph_free(import->owned_graph);
	if (import->shm_ptr != NULL)
		shmdt(import->shm_ptr);
	memset(import, 0, sizeof(*import));
}

static int32_t g2d_import_from_shm(g2d_import_t* import, const g2d_blit_req_t* req) {
	uint8_t* shm;
	uint32_t stride;
	uint32_t min_size;
	uint32_t y;
	graph_t* packed;

	if (import == NULL || req == NULL)
		return -1;
	if (req->src_shm_id < 0 || req->src_w == 0 || req->src_h == 0 || req->src_format != G2D_FMT_ARGB8888)
		return -1;

	stride = req->src_stride;
	if (stride == 0)
		stride = req->src_w * 4;
	if (stride < req->src_w * 4)
		return -1;

	min_size = stride * req->src_h;
	if (req->src_size < min_size)
		return -1;

	shm = (uint8_t*)shmat(req->src_shm_id, 0, 0);
	if (shm == (void*)-1)
		return -1;

	g2d_import_init(import);
	import->shm_ptr = shm;
	if (stride == req->src_w * 4) {
		graph_init(&import->view, (const uint32_t*)shm, req->src_w, req->src_h);
		return 0;
	}

	packed = graph_new(NULL, req->src_w, req->src_h);
	if (packed == NULL || packed->buffer == NULL) {
		if (packed != NULL)
			graph_free(packed);
		g2d_import_release(import);
		return -1;
	}

	for (y = 0; y < req->src_h; ++y) {
		memcpy(((uint8_t*)packed->buffer) + y * req->src_w * 4,
				shm + y * stride,
				req->src_w * 4);
	}
	import->owned_graph = packed;
	import->view = *packed;
	return 0;
}

static int32_t g2d_state_init(g2d_state_t* state, uint32_t width, uint32_t height, uint32_t dep) {
	(void)dep;
	if (state == NULL)
		return -1;

	memset(state, 0, sizeof(*state));
	if (g2d_surface_init(&state->dst, width, height) != 0)
		return -1;

	state->clear_color = 0xff000000u;
	memset(&state->stats, 0, sizeof(state->stats));
	state->stats.backend = G2D_BACKEND_SOFT_NV12;
	return 0;
}

static void g2d_state_release(g2d_state_t* state) {
	if (state == NULL)
		return;
	g2d_surface_release(&state->dst);
}

static int32_t g2d_state_clear(g2d_state_t* state, uint32_t color) {
	if (state == NULL)
		return -1;
	if (g2d_surface_clear(&state->dst, color) != 0)
		return -1;

	state->clear_color = color;
	state->stats.soft_clear_ops++;
	return 0;
}

static int32_t g2d_state_fill_rect(g2d_state_t* state, const g2d_fill_req_t* req) {
	if (state == NULL)
		return -1;
	if (g2d_surface_fill_rect(&state->dst, req) != 0)
		return -1;

	state->stats.soft_fill_ops++;
	return 0;
}

static int32_t g2d_state_blit(g2d_state_t* state, const g2d_blit_req_t* req, graph_t* src, uint8_t use_alpha) {
	if (state == NULL)
		return -1;
	if (g2d_surface_blit(&state->dst, src, req, use_alpha) != 0)
		return -1;

	if (use_alpha != 0)
		state->stats.soft_alpha_blit_ops++;
	else
		state->stats.soft_blit_ops++;
	return 0;
}

static int32_t g2d_state_present(g2d_state_t* state) {
	if (state == NULL || state->dst.graph == NULL)
		return -1;
	state->stats.soft_present_ops++;
	return 0;
}

static int32_t g2d_state_get_pixel(g2d_state_t* state, int32_t x, int32_t y, uint32_t* pixel) {
	if (state == NULL)
		return -1;
	return g2d_surface_get_pixel(&state->dst, x, y, pixel);
}

static int32_t g2d_reply_info(proto_t* ret, const g2d_state_t* state) {
	g2d_info_t info;

	if (ret == NULL || state == NULL || state->dst.graph == NULL)
		return -1;

	memset(&info, 0, sizeof(info));
	info.width = state->dst.width;
	info.height = state->dst.height;
	info.depth = 32;
	info.format = G2D_FMT_ARGB8888;
	info.backend = G2D_BACKEND_SOFT_NV12;
	PF->init(ret)->add(ret, &info, sizeof(info));
	return 0;
}

static int32_t g2d_reply_stats(proto_t* ret, const g2d_state_t* state) {
	g2d_stats_t stats;

	if (ret == NULL || state == NULL)
		return -1;

	stats = state->stats;
	stats.backend = G2D_BACKEND_SOFT_NV12;
	stats.vc_ready = 0;
	PF->init(ret)->add(ret, &stats, sizeof(stats));
	return 0;
}

static int32_t g2d_reply_pixel(proto_t* ret, uint32_t pixel) {
	if (ret == NULL)
		return -1;
	PF->init(ret)->add(ret, &pixel, sizeof(pixel));
	return 0;
}

static int32_t g2dd_handle_get_info(proto_t* ret, g2d_state_t* state) {
	return g2d_reply_info(ret, state);
}

static int32_t g2dd_handle_clear(proto_t* in, g2d_state_t* state) {
	uint32_t color;

	if (in == NULL || state == NULL)
		return -1;
	if (proto_read_to(in, &color, sizeof(color)) != sizeof(color))
		return -1;
	return g2d_state_clear(state, color);
}

static int32_t g2dd_handle_fill_rect(proto_t* in, g2d_state_t* state) {
	g2d_fill_req_t req;

	if (in == NULL || state == NULL)
		return -1;
	if (proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;
	return g2d_state_fill_rect(state, &req);
}

static int32_t g2dd_handle_blit(proto_t* in, g2d_state_t* state, uint8_t use_alpha) {
	g2d_blit_req_t req;
	g2d_import_t import;
	int32_t ret;

	if (in == NULL || state == NULL)
		return -1;
	if (proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;

	g2d_import_init(&import);
	if (g2d_import_from_shm(&import, &req) != 0)
		return -1;

	ret = g2d_state_blit(state, &req, &import.view, use_alpha);
	g2d_import_release(&import);
	return ret;
}

static int32_t g2dd_handle_present(g2d_state_t* state) {
	return g2d_state_present(state);
}

static int32_t g2dd_handle_get_pixel(proto_t* in, proto_t* ret, g2d_state_t* state) {
	g2d_pixel_req_t req;
	uint32_t pixel;

	if (in == NULL || ret == NULL || state == NULL)
		return -1;
	if (proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;
	if (g2d_state_get_pixel(state, req.x, req.y, &pixel) != 0)
		return -1;
	return g2d_reply_pixel(ret, pixel);
}

static int32_t g2dd_handle_get_stats(proto_t* ret, g2d_state_t* state) {
	return g2d_reply_stats(ret, state);
}

static char* g2d_strdup(const char* s) {
	size_t len;
	char* ret;

	if (s == NULL)
		return NULL;
	len = strlen(s);
	ret = (char*)malloc(len + 1);
	if (ret == NULL)
		return NULL;
	memcpy(ret, s, len + 1);
	return ret;
}

static char* g2d_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
	(void)dev;
	(void)from_pid;
	g2d_state_t* state = (g2d_state_t*)p;

	if (argc <= 0 || argv == NULL || argv[0] == NULL || state == NULL || state->dst.graph == NULL)
		return NULL;

	if (strcmp(argv[0], "info") == 0) {
		static char info[96];
		snprintf(info, sizeof(info), "%dx%d argb8888 via soft",
				state->dst.graph->w, state->dst.graph->h);
		return g2d_strdup(info);
	}
	if (strcmp(argv[0], "present") == 0)
		return g2dd_handle_present(state) == 0 ? g2d_strdup("ok") : NULL;
	if (strcmp(argv[0], "clear") == 0 && argc > 1) {
		uint32_t color = (uint32_t)strtoul(argv[1], NULL, 0);
		return g2d_state_clear(state, color) == 0 ? g2d_strdup("ok") : NULL;
	}
	return NULL;
}

static int g2d_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)dev;
	(void)from_pid;
	g2d_state_t* state = (g2d_state_t*)p;

	if (state == NULL)
		return -1;

	switch (cmd) {
	case G2D_DEV_CNTL_GET_INFO:
		return g2dd_handle_get_info(ret, state);
	case G2D_DEV_CNTL_CLEAR:
		return g2dd_handle_clear(in, state);
	case G2D_DEV_CNTL_FILL_RECT:
		return g2dd_handle_fill_rect(in, state);
	case G2D_DEV_CNTL_BLIT:
		return g2dd_handle_blit(in, state, 0);
	case G2D_DEV_CNTL_BLIT_ALPHA:
		return g2dd_handle_blit(in, state, 1);
	case G2D_DEV_CNTL_PRESENT:
		return g2dd_handle_present(state);
	case G2D_DEV_CNTL_GET_PIXEL:
		return g2dd_handle_get_pixel(in, ret, state);
	case G2D_DEV_CNTL_GET_STATS:
		return g2dd_handle_get_stats(ret, state);
	default:
		return -1;
	}
}

static void read_config(const char* conf_file, uint32_t* w, uint32_t* h, uint32_t* dep) {
	json_var_t* conf_var;

	if (conf_file == NULL || conf_file[0] == 0)
		conf_file = "/etc/framebuffer.json";

	conf_var = json_parse_file(conf_file);
	*w = (uint32_t)json_get_int_def(conf_var, "width", 1024);
	*h = (uint32_t)json_get_int_def(conf_var, "height", 768);
	*dep = (uint32_t)json_get_int_def(conf_var, "depth", 32);
	if (conf_var != NULL)
		json_var_unref(conf_var);
}

static int doargs(int argc, char* argv[], const char** conf_file) {
	int c = 0;

	while (c != -1) {
		c = getopt(argc, argv, "c:");
		if (c == -1)
			break;
		switch (c) {
		case 'c':
			*conf_file = optarg;
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	const char* conf_file = "/etc/framebuffer.json";
	const char* mnt_point;
	g2d_state_t state;
	vdevice_t dev;
	uint32_t w = 0;
	uint32_t h = 0;
	uint32_t dep = 32;
	int opti;

	opti = doargs(argc, argv, &conf_file);
	mnt_point = (opti < argc && opti >= 0) ? argv[opti] : "/dev/g2d";

	read_config(conf_file, &w, &h, &dep);
	if (g2d_state_init(&state, w, h, dep) != 0)
		return -1;

	memset(&dev, 0, sizeof(dev));
	strcpy(dev.name, "g2d");
	dev.dev_cntl = g2d_dev_cntl;
	dev.cmd = g2d_cmd;
	dev.extra_data = &state;

	if (device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666) != 0) {
		g2d_state_release(&state);
		return -1;
	}

	g2d_state_release(&state);
	return 0;
}
