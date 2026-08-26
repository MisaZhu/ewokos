#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <ewoksys/proto.h>
#include <ewoksys/klog.h>
#include <ewoksys/vdevice.h>
#include <bsp/bsp_g2d.h>
#include <g2dclient/g2dclient.h>

/* stateless g2d service: the driver owns no canvas. every request
   carries its canvases (g2d_canvas_t) either as keyed shm segment ids
   the driver attaches to, or as dma addresses usable directly through
   the identity dma window. the driver operates in place and detaches
   shm canvases when done. */

typedef struct {
	uint32_t* buffer;
	uint32_t width;
	uint32_t height;
	uint8_t dma; /* dma canvas: buffer is a dma address, no shmdt */
} g2d_attached_t;

static int32_t g2d_norm_degree(int32_t degree) {
	return ((degree % 360) + 360) % 360;
}

/* attach a request canvas; rejects undersized segments so the mapping
   can never be written past its end */
static int32_t g2d_attach(const g2d_canvas_t* canvas, g2d_attached_t* at) {
	void* p;

	if(canvas == NULL || at == NULL)
		return -1;
	if(canvas->w == 0 || canvas->h == 0)
		return -1;
	if(canvas->size < canvas->w * canvas->h * sizeof(uint32_t))
		return -1;

	if(canvas->dma != 0) {
		/* dma canvas: addr is the dma address of the buffer. the kernel
		   maps the dma window identity (vaddr == dma addr) into every
		   process, so the driver can use it directly */
		if(canvas->addr == 0)
			return -1;
		at->buffer = (uint32_t*)(uintptr_t)canvas->addr;
		at->width = canvas->w;
		at->height = canvas->h;
		at->dma = 1;
		return 0;
	}

	if(canvas->shm_id <= 0)
		return -1;
	p = shmat(canvas->shm_id, 0, 0);
	if(p == (void*)-1)
		return -1;
	at->buffer = (uint32_t*)p;
	at->width = canvas->w;
	at->height = canvas->h;
	at->dma = 0;
	return 0;
}

static void g2d_detach(const g2d_attached_t* at) {
	if(at != NULL && at->buffer != NULL && at->dma == 0)
		shmdt(at->buffer);
}

/* alpha fill: same blend as graph_pixel_argb_raw, clipped to the
   canvas bounds */
static void g2d_fill_alpha(uint32_t* buf, int32_t bw, int32_t bh,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	uint8_t a = (color >> 24) & 0xff;
	uint8_t r = (color >> 16) & 0xff;
	uint8_t g = (color >> 8) & 0xff;
	uint8_t b = color & 0xff;
	uint32_t inv_a = 255 - a;
	int32_t px;
	int32_t py;

	if(a == 0)
		return;
	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(w <= 0 || h <= 0 || x >= bw || y >= bh)
		return;
	if(x + w > bw) w = bw - x;
	if(y + h > bh) h = bh - y;

	for(py = y; py < y + h; py++) {
		for(px = x; px < x + w; px++) {
			uint32_t oc = buf[py * bw + px];
			uint8_t oa = (oc >> 24) & 0xff;
			uint8_t orr = (oc >> 16) & 0xff;
			uint8_t og = (oc >> 8) & 0xff;
			uint8_t ob = oc & 0xff;

			oa = oa + (uint8_t)((255 - oa) * a / 255);
			orr = (uint8_t)((r * a + orr * inv_a) / 255);
			og = (uint8_t)((g * a + og * inv_a) / 255);
			ob = (uint8_t)((b * a + ob * inv_a) / 255);
			buf[py * bw + px] = ((uint32_t)oa << 24) | ((uint32_t)orr << 16) |
					((uint32_t)og << 8) | ob;
		}
	}
}

static int32_t g2dd_handle_fill_rect(proto_t* in) {
	g2d_fill_req_t req;
	g2d_attached_t dst;
	int32_t ret;

	if(in == NULL)
		return -1;
	if(proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;

	if(g2d_attach(&req.dst, &dst) != 0)
		return -1;
	if(((req.color >> 24) & 0xff) == 0xff) {
		bsp_g2d_fill(dst.buffer, (int32_t)dst.width, (int32_t)dst.height,
				req.rect.x, req.rect.y, req.rect.w, req.rect.h, req.color,
				dst.dma);
	}
	else {
		g2d_fill_alpha(dst.buffer, (int32_t)dst.width, (int32_t)dst.height,
				req.rect.x, req.rect.y, req.rect.w, req.rect.h, req.color);
	}
	g2d_detach(&dst);
	ret = 0;
	return ret;
}

static int32_t g2d_blit_render(const g2d_attached_t* dst,
		uint32_t* src_buf, int32_t src_w, int32_t src_h,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh, uint8_t src_dma,
		const g2d_blit_req_t* req, uint8_t use_alpha) {
	if(dst == NULL || dst->buffer == NULL || src_buf == NULL || req == NULL)
		return -1;
	if(req->dw <= 0 || req->dh <= 0)
		return -1;
    slog("g2d_blit_render src:%d x %d, dst: %d x %d, alpha: %d\n", src_w, src_h, dst->width, dst->height, use_alpha);

	if(use_alpha != 0) {
		bsp_g2d_blt_alpha(src_buf, src_w, src_h, sx, sy, sw, sh,
				dst->buffer, (int32_t)dst->width, (int32_t)dst->height,
				req->dx, req->dy, req->dw, req->dh, req->alpha,
				src_dma, dst->dma);
	}
	else {
		bsp_g2d_blt(src_buf, src_w, src_h, sx, sy, sw, sh,
				dst->buffer, (int32_t)dst->width, (int32_t)dst->height,
				req->dx, req->dy, req->dw, req->dh,
				src_dma, dst->dma);
	}
	return 0;
}

/* blit src crop into the dst canvas; with rotation the crop goes into
   temp buffers first, then the rotated result is scaled into the dst
   rect */
static int32_t g2dd_handle_blit(proto_t* in, uint8_t use_alpha) {
	g2d_blit_req_t req;
	g2d_attached_t dst;
	g2d_attached_t src;
	uint32_t* cropped;
	uint32_t* rotated;
	int32_t degree;
	int32_t rw;
	int32_t rh;
	int32_t ret;

	if(in == NULL)
		return -1;
	if(proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;
	if(req.sx < 0 || req.sy < 0 || req.sw <= 0 || req.sh <= 0)
		return -1;

	if(g2d_attach(&req.dst, &dst) != 0)
		return -1;
	if(g2d_attach(&req.src, &src) != 0) {
		g2d_detach(&dst);
		return -1;
	}

	ret = -1;
	if(req.sx + req.sw > (int32_t)src.width ||
			req.sy + req.sh > (int32_t)src.height)
		goto done;

	/* rotate is clockwise degrees, normalized to [0, 360) */
	degree = g2d_norm_degree(req.rotate);

	/* no rotation: blt scales and clips the crop rect directly */
	if(degree == 0) {
		ret = g2d_blit_render(&dst, src.buffer,
				(int32_t)src.width, (int32_t)src.height,
				req.sx, req.sy, req.sw, req.sh, src.dma, &req, use_alpha);
		goto done;
	}

	/* rotated path: crop into a temp surface, then rotate into another;
	   the temp surfaces are plain malloc memory, never dma */
	cropped = (uint32_t*)malloc((size_t)req.sw * req.sh * sizeof(uint32_t));
	if(cropped == NULL)
		goto done;
	bsp_g2d_blt(src.buffer, (int32_t)src.width, (int32_t)src.height,
			req.sx, req.sy, req.sw, req.sh,
			cropped, req.sw, req.sh, 0, 0, req.sw, req.sh,
			src.dma, 0);

	bsp_g2d_rotated_size(req.sw, req.sh, degree, &rw, &rh);
	if(rw <= 0 || rh <= 0) {
		free(cropped);
		goto done;
	}
	rotated = (uint32_t*)malloc((size_t)rw * rh * sizeof(uint32_t));
	if(rotated == NULL) {
		free(cropped);
		goto done;
	}
	bsp_g2d_rotate(cropped, req.sw, req.sh, rotated, rw, rh, degree, 0, 0);
	free(cropped);

	ret = g2d_blit_render(&dst, rotated, rw, rh,
			0, 0, rw, rh, 0, &req, use_alpha);
	free(rotated);

done:
	g2d_detach(&src);
	g2d_detach(&dst);
	return ret;
}

/* rotate the src canvas into the dst canvas; the dst canvas size must
   match the rotated size computed by the backend */
static int32_t g2dd_handle_rotate(proto_t* in) {
	g2d_rotate_req_t req;
	g2d_attached_t src;
	g2d_attached_t dst;
	int32_t degree;
	int32_t rw;
	int32_t rh;
	int32_t ret;

	if(in == NULL)
		return -1;
	if(proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;

	degree = g2d_norm_degree(req.rotate);
	if(degree == 0)
		return -1;

	if(g2d_attach(&req.src, &src) != 0)
		return -1;
	if(g2d_attach(&req.dst, &dst) != 0) {
		g2d_detach(&src);
		return -1;
	}

	ret = -1;
	bsp_g2d_rotated_size((int32_t)src.width, (int32_t)src.height,
			degree, &rw, &rh);
	if(rw == (int32_t)dst.width && rh == (int32_t)dst.height) {
		bsp_g2d_rotate(src.buffer, (int32_t)src.width, (int32_t)src.height,
				dst.buffer, rw, rh, degree, src.dma, dst.dma);
		ret = 0;
	}

	g2d_detach(&src);
	g2d_detach(&dst);
	return ret;
}

static int32_t g2dd_handle_scale_to(proto_t* in) {
	g2d_scale_to_req_t req;
	g2d_attached_t src;
	g2d_attached_t dst;

	if(in == NULL)
		return -1;
	if(proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;

	if(g2d_attach(&req.src, &src) != 0)
		return -1;
	if(g2d_attach(&req.dst, &dst) != 0) {
		g2d_detach(&src);
		return -1;
	}

    slog("g2dd_handle_scale_to %dx%d\n", src.width, src.height);
	bsp_g2d_scale_to(src.buffer, (int32_t)src.width, (int32_t)src.height,
			dst.buffer, (int32_t)dst.width, (int32_t)dst.height,
			src.dma, dst.dma);

	g2d_detach(&src);
	g2d_detach(&dst);
	return 0;
}

static char* g2d_strdup(const char* s) {
	size_t len;
	char* ret;

	if(s == NULL)
		return NULL;
	len = strlen(s);
	ret = (char*)malloc(len + 1);
	if(ret == NULL)
		return NULL;
	memcpy(ret, s, len + 1);
	return ret;
}

static char* g2d_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
	(void)dev;
	(void)from_pid;
	(void)p;

	if(argc <= 0 || argv == NULL || argv[0] == NULL)
		return NULL;

	if(strcmp(argv[0], "info") == 0)
		return g2d_strdup("stateless argb8888 shm canvases via soft");
	return NULL;
}

static int g2d_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)dev;
	(void)from_pid;
	(void)ret;
	(void)p;

	switch (cmd) {
	case G2D_DEV_CNTL_FILL_RECT:
		return g2dd_handle_fill_rect(in);
	case G2D_DEV_CNTL_BLIT:
		return g2dd_handle_blit(in, 0);
	case G2D_DEV_CNTL_BLIT_ALPHA:
		return g2dd_handle_blit(in, 1);
	case G2D_DEV_CNTL_ROTATE:
		return g2dd_handle_rotate(in);
	case G2D_DEV_CNTL_SCALE_TO:
		return g2dd_handle_scale_to(in);
	default:
		return -1;
	}
}

int main(int argc, char** argv) {
	const char* mnt_point;
	vdevice_t dev;

	mnt_point = (argc > 1) ? argv[1] : "/dev/g2d";

	if(bsp_g2d_init() != 0)
		return -1;

	memset(&dev, 0, sizeof(dev));
	strcpy(dev.desc, "g2d");
	dev.dev_cntl = g2d_dev_cntl;
	dev.cmd = g2d_cmd;
	dev.extra_data = NULL;

	if(device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666) != 0)
		return -1;
	return 0;
}
