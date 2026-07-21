#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vdevice.h>
#include <sys/shm.h>
#include <fb/fb.h>
#include <fbd/fbd.h>
#include <graph/graph_image.h>
#include <tinyjson/tinyjson.h>

typedef struct {
	uint32_t size;
	uint8_t* shm;
	int32_t  shm_id;
} fb_dma_t;

static fbinfo_t _fbinfo = {0};
static int32_t _rotate = 0;
static float _zoom = 1.0;
static int32_t _zwidth;
static int32_t _zheight;
static fbd_t* _fbd = NULL;
static char _logo[256] = {0};

static int fb_fcntl(vdevice_t* dev, int fd,
		int from_pid,
		fsinfo_t* info,
		int cmd,
		proto_t* in,
		proto_t* out,
		void* p) {

	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)in;
	(void)p;
	if(cmd == FB_CNTL_GET_INFO) { //get fb size
		if(_rotate == G_ROTATE_270 || _rotate == G_ROTATE_90)
			PF->addi(out, _zheight)->addi(out, _zwidth)->addi(out, _fbinfo.depth);
		else
			PF->addi(out, _zwidth)->addi(out, _zheight)->addi(out, _fbinfo.depth);
	}
	return 0;
}

static void draw_bg(graph_t* g) {
	//graph_gradation(g, 0, 0, g->w, g->h, 0xff8888ff, 0xff000000, true);
	graph_gradation(g, 0, 0, g->w, g->h, 0xff444488, 0xff000000, true);
#if __aarch64__
	graph_t* logo = graph_image_new("/usr/system/icons/64bits.png");
	if(logo != NULL) {
		graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
				g, g->w - logo->w - 10, 10, logo->w, logo->h, 0xff);
		graph_free(logo);
	}
#endif
}

static void default_splash(graph_t* g, const char* logo_fname) {
	draw_bg(g);
	graph_t* logo = graph_image_new(logo_fname);
	if(logo != NULL) {
		graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
				g, (g->w-logo->w)/2, (g->h-logo->h)/2, logo->w, logo->h, 0xff);
		graph_free(logo);
	}
}

/*
 * Rotation straight into the scan-out buffer.
 *
 * Scan-out mappings are typically Normal Non-Cacheable: stores only
 * merge into DRAM bursts when they hit the write-combine buffers
 * sequentially, so every rotate below walks the DESTINATION row-major.
 * The source lives in cacheable shm, so its strided reads are absorbed
 * by L1/L2 (each 64-byte line covers 16 src pixels reused across 16
 * dst rows).
 *
 * 4x4 micro-tiles: the inner block keeps at most 4 open WC streams and
 * compiles to NEON/SSE 4x4 transposes at -O2; scalar tail loops cover
 * non-multiple-of-4 geometries.
 */
static uint32_t rot90_to_fb(const fbinfo_t* fbi, const graph_t* g) {
	/* dst[y][x] = src[sh-1-x][y]; dst is sh wide, sw tall */
	uint32_t sw = (uint32_t)g->w, sh = (uint32_t)g->h;
	uint32_t wf = sw & ~3U, hf = sh & ~3U;
	uint8_t* dst_base = (uint8_t*)(uintptr_t)fbi->pointer +
			fbi->yoffset * fbi->pitch + fbi->xoffset * 4;
	const uint32_t* src = g->buffer;
	uint32_t x, y;

	for (y = 0; y < wf; y += 4) {
		uint32_t* d0 = (uint32_t*)(dst_base + (y + 0) * fbi->pitch);
		uint32_t* d1 = (uint32_t*)(dst_base + (y + 1) * fbi->pitch);
		uint32_t* d2 = (uint32_t*)(dst_base + (y + 2) * fbi->pitch);
		uint32_t* d3 = (uint32_t*)(dst_base + (y + 3) * fbi->pitch);
		for (x = 0; x < hf; x += 4) {
			const uint32_t* s0 = src + (sh - 1 - x) * sw + y;
			const uint32_t* s1 = s0 - sw;
			const uint32_t* s2 = s1 - sw;
			const uint32_t* s3 = s2 - sw;
			d0[x] = s0[0]; d0[x+1] = s1[0]; d0[x+2] = s2[0]; d0[x+3] = s3[0];
			d1[x] = s0[1]; d1[x+1] = s1[1]; d1[x+2] = s2[1]; d1[x+3] = s3[1];
			d2[x] = s0[2]; d2[x+1] = s1[2]; d2[x+2] = s2[2]; d2[x+3] = s3[2];
			d3[x] = s0[3]; d3[x+1] = s1[3]; d3[x+2] = s2[3]; d3[x+3] = s3[3];
		}
		for (; x < sh; ++x) {
			const uint32_t* s = src + (sh - 1 - x) * sw + y;
			d0[x] = s[0]; d1[x] = s[1]; d2[x] = s[2]; d3[x] = s[3];
		}
	}
	for (; y < sw; ++y) {
		uint32_t* d = (uint32_t*)(dst_base + y * fbi->pitch);
		for (x = 0; x < sh; ++x)
			d[x] = src[(sh - 1 - x) * sw + y];
	}
	return sw * sh * 4U;
}

static uint32_t rot270_to_fb(const fbinfo_t* fbi, const graph_t* g) {
	/* dst[y][x] = src[x][sw-1-y]; dst is sh wide, sw tall */
	uint32_t sw = (uint32_t)g->w, sh = (uint32_t)g->h;
	uint32_t wf = sw & ~3U, hf = sh & ~3U;
	uint8_t* dst_base = (uint8_t*)(uintptr_t)fbi->pointer +
			fbi->yoffset * fbi->pitch + fbi->xoffset * 4;
	const uint32_t* src = g->buffer;
	uint32_t x, y;

	for (y = 0; y < wf; y += 4) {
		uint32_t* d0 = (uint32_t*)(dst_base + (y + 0) * fbi->pitch);
		uint32_t* d1 = (uint32_t*)(dst_base + (y + 1) * fbi->pitch);
		uint32_t* d2 = (uint32_t*)(dst_base + (y + 2) * fbi->pitch);
		uint32_t* d3 = (uint32_t*)(dst_base + (y + 3) * fbi->pitch);
		for (x = 0; x < hf; x += 4) {
			/* sk[3-i] == src[x+k][sw-1-(y+i)] */
			const uint32_t* s0 = src + (x + 0) * sw + (sw - 4 - y);
			const uint32_t* s1 = s0 + sw;
			const uint32_t* s2 = s1 + sw;
			const uint32_t* s3 = s2 + sw;
			d0[x] = s0[3]; d0[x+1] = s1[3]; d0[x+2] = s2[3]; d0[x+3] = s3[3];
			d1[x] = s0[2]; d1[x+1] = s1[2]; d1[x+2] = s2[2]; d1[x+3] = s3[2];
			d2[x] = s0[1]; d2[x+1] = s1[1]; d2[x+2] = s2[1]; d2[x+3] = s3[1];
			d3[x] = s0[0]; d3[x+1] = s1[0]; d3[x+2] = s2[0]; d3[x+3] = s3[0];
		}
		for (; x < sh; ++x) {
			const uint32_t* s = src + x * sw + (sw - 1 - y);
			d0[x] = s[0]; d1[x] = s[-1]; d2[x] = s[-2]; d3[x] = s[-3];
		}
	}
	for (; y < sw; ++y) {
		uint32_t* d = (uint32_t*)(dst_base + y * fbi->pitch);
		for (x = 0; x < sh; ++x)
			d[x] = src[x * sw + (sw - 1 - y)];
	}
	return sw * sh * 4U;
}

static uint32_t rot180_to_fb(const fbinfo_t* fbi, const graph_t* g) {
	/* dst[y][x] = src[sh-1-y][sw-1-x]; same geometry as the panel */
	uint32_t sw = (uint32_t)g->w, sh = (uint32_t)g->h;
	uint8_t* dst_base = (uint8_t*)(uintptr_t)fbi->pointer +
			fbi->yoffset * fbi->pitch + fbi->xoffset * 4;
	const uint32_t* src = g->buffer;

	for (uint32_t y = 0; y < sh; ++y) {
		uint32_t* d = (uint32_t*)(dst_base + y * fbi->pitch);
		const uint32_t* s = src + (sh - y) * sw - 1;
		for (uint32_t x = 0; x < sw; ++x)
			d[x] = s[-(int32_t)x];
	}
	return sw * sh * 4U;
}

uint32_t fbd_rotate_to(const fbinfo_t* fbinfo, const graph_t* g, int rotate) {
	if (fbinfo == NULL || g == NULL || g->buffer == NULL)
		return 0;
	if (fbinfo->pointer == 0 || fbinfo->depth != 32)
		return 0;

	if (rotate == G_ROTATE_90 &&
			(uint32_t)g->h == fbinfo->width && (uint32_t)g->w == fbinfo->height)
		return rot90_to_fb(fbinfo, g);
	if (rotate == G_ROTATE_270 &&
			(uint32_t)g->h == fbinfo->width && (uint32_t)g->w == fbinfo->height)
		return rot270_to_fb(fbinfo, g);
	if (rotate == G_ROTATE_180 &&
			(uint32_t)g->w == fbinfo->width && (uint32_t)g->h == fbinfo->height)
		return rot180_to_fb(fbinfo, g);
	return 0;
}

static graph_t* _rotate_g = NULL; /* cached rotate buffer, allocated once */

static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	if(fbinfo->depth != 32 && fbinfo->depth != 16)
		return 0;

	int zoomed = (_zoom > 0.0 && _zoom != 8.0 && _zoom != 1.0);
	graph_t g;
	if(rotate == G_ROTATE_270 || rotate == G_ROTATE_90)
		graph_init(&g, buf, _zheight, _zwidth);
	else
		graph_init(&g, buf, _zwidth, _zheight);

	/* fast path: driver rotates by itself, straight into the scan-out
	 * buffer. Skips the intermediate rotate buffer AND the extra
	 * full-frame copy the generic path below needs. */
	if(rotate != G_ROTATE_0 && !zoomed && _fbd->flush_rotate != NULL) {
		uint32_t res = _fbd->flush_rotate(fbinfo, &g, rotate);
		if(res > 0)
			return res;
	}

	graph_t* tmp_g = &g;
	if(rotate == G_ROTATE_90 || rotate == G_ROTATE_270 || rotate == G_ROTATE_180) {
		if(_rotate_g == NULL)
			_rotate_g = graph_new(NULL, _zwidth, _zheight);
		if(_rotate_g != NULL) {
			graph_rotate_to(&g, _rotate_g, rotate);
			tmp_g = _rotate_g;
		}
	}

	if(zoomed) {
		graph_t* gzoom = graph_new(NULL, fbinfo->width, fbinfo->height);
		graph_scale_tof(tmp_g, gzoom, _zoom);
		tmp_g = gzoom;
	}

	uint32_t res = _fbd->flush(fbinfo, tmp_g);
	if(tmp_g != &g && tmp_g != _rotate_g)
		graph_free(tmp_g);
	return res;
}

static void init_graph(fb_dma_t* dma) {
	graph_t g;
	if(_rotate == G_ROTATE_270 || _rotate == G_ROTATE_90)
		graph_init(&g, (const uint32_t*)dma->shm, _zheight, _zwidth);
	else
		graph_init(&g, (const uint32_t*)dma->shm, _zwidth, _zheight);

	if(_fbd->splash != NULL)
		_fbd->splash(&g, _logo);
	else
		default_splash(&g, _logo);
	flush(&_fbinfo, dma->shm, dma->size, _rotate);
}

static int fb_dma_init(fb_dma_t* dma) {
	memset(dma, 0, sizeof(fb_dma_t));
	uint32_t sz = _zwidth * _zheight * 4;
	key_t key = (((int32_t)dma) << 16) | getpid(); 
	dma->shm_id = shmget(key, sz + 1, 0666 | IPC_CREAT | IPC_EXCL); //one more byte (head) for busy flag 
	if(dma->shm_id == -1)
		return -1;
	dma->shm = shmat(dma->shm_id, 0, 0); //one more byte (head) for busy flag 
	if(dma->shm == NULL)
		return -1;
	//dma->size = _fbinfo.size_max;
	memset(dma->shm, 0, sz+1);
	dma->size = sz;
	init_graph(dma);
	return 0;
}

static void fb_get_info() {
	fbinfo_t* info = _fbd->get_info();
	memcpy(&_fbinfo, info, sizeof(fbinfo_t));
	_zwidth = _fbinfo.width / _zoom;
	_zheight = _fbinfo.height / _zoom;
}

static int fb_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)dev;
	(void)from_pid;
	(void)ret;
	(void)p;

	if(cmd == FB_DEV_CNTL_SET_INFO) { //set fb size and bpp
		int w = proto_read_int(in);
		int h = proto_read_int(in);
		int bpp = proto_read_int(in);
		if(_fbd->init(w, h, bpp) != 0)
			return -1;
		fb_get_info();
	}
	else if(cmd == FB_DEV_CNTL_GET_INFO) {
		if(_rotate == G_ROTATE_270 || _rotate == G_ROTATE_90)
			PF->addi(ret, _zheight)->addi(ret, _zwidth)->addi(ret, _fbinfo.depth);
		else
			PF->addi(ret, _zwidth)->addi(ret, _zheight)->addi(ret, _fbinfo.depth);
	}	
	return 0;
}

static int32_t do_flush(fb_dma_t* dma) {
	uint8_t* buf = (uint8_t*)dma->shm;
	if(buf == NULL)
		return -1;

	uint32_t size = dma->size;
	buf[size] = 1; //busy
	int32_t res = flush(&_fbinfo, buf, size, _rotate);
	buf[size] = 0; //done
	return res;
}

/*return
0: error;
-1: resized;
>0: size flushed*/
static int do_fb_flush(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)info;

	fb_dma_t* dma = (fb_dma_t*)p;
	return do_flush(dma);
}

static int32_t fb_dma(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int* size, void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;
	*size = dma->size;
	return dma->shm_id;
}

static void read_config(const char* conf_file, uint32_t* w, uint32_t* h, uint8_t* dep, int32_t* rotate, float* zoom) {
	if(conf_file == NULL || conf_file[0] == 0)
		conf_file = "/etc/framebuffer.json";
	json_var_t *conf_var = json_parse_file(conf_file);	

	const char* v = json_get_str_def(conf_var, "logo", "/usr/system/images/logos/ewokos.png");
	if(v[0] != 0) 
		strncpy(_logo, v, 255);

	*zoom = json_get_float_def(conf_var, "zoom", 1.0);
	*w = json_get_int_def(conf_var, "width", 0);
	*h = json_get_int_def(conf_var, "height", 0);
	*dep = json_get_int_def(conf_var, "depth", 32);
	*rotate = json_get_int_def(conf_var, "rotate", 0);

	if(*zoom <= 0)
		*zoom = 1.0;
	else if(*zoom > 8.0)
		*zoom = 8.0;

	if(conf_var != NULL)
		json_var_unref(conf_var);
}


static int fb_dev_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)p;

	if(_fbd->read == NULL)
		return 0;
	return _fbd->read(buf, size);
}

int fbd_run(fbd_t* fbd, const char* mnt_name,
		uint32_t def_w, uint32_t def_h, const char* conf_file) {
	_fbd = fbd;
	uint32_t w = def_w, h = def_h;
	_zoom = 1.0;
	uint8_t dep = 32;
	_rotate = G_ROTATE_0;

	read_config(conf_file, &w, &h, &dep, &_rotate, &_zoom);

	fb_dma_t dma;
	dma.shm = NULL;
	if(fbd->init(w, h, dep) != 0)
		return -1;
	fb_get_info();
	
	if(fb_dma_init(&dma) != 0)
		return -1;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "framebuffer");
	dev.dma = fb_dma;
	dev.flush = do_fb_flush;
	dev.fcntl = fb_fcntl;
	dev.dev_cntl = fb_dev_cntl;
	dev.read = fb_dev_read;

	dev.extra_data = &dma;

	add_display_fb_dev("/dev/display", mnt_name);
	device_run(&dev, mnt_name, FS_TYPE_CHAR, 0666);
	shmdt(dma.shm);
	return 0;
}
