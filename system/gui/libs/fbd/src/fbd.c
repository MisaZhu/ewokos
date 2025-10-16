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
#include <graph/graph_png.h>
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

static int fb_fcntl(int fd, 
		int from_pid,
		fsinfo_t* node, 
		int cmd, 
		proto_t* in, 
		proto_t* out,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)node;
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
	graph_gradation(g, 0, 0, g->w, g->h, 0xff8888ff, 0xff000000, true);
#if __aarch64__
	graph_t* logo = png_image_new("/usr/system/icons/64bits.png");
	if(logo != NULL) {
		graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
				g, g->w - logo->w - 10, 10, logo->w, logo->h, 0xff);
		graph_free(logo);
	}
#endif
}

static void default_splash(graph_t* g, const char* logo_fname) {
	draw_bg(g);
	graph_t* logo = png_image_new(logo_fname);
	if(logo != NULL) {
		graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
				g, (g->w-logo->w)/2, (g->h-logo->h)/2, logo->w, logo->h, 0xff);
		graph_free(logo);
	}
}

static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	if(fbinfo->depth != 32 && fbinfo->depth != 16)
		return 0;

	graph_t g;

	graph_t* gr = NULL;
	if(rotate == G_ROTATE_270 || rotate == G_ROTATE_90) {
		graph_init(&g, buf, _zheight, _zwidth);
		gr = graph_new(NULL, _zwidth, _zheight);
	}
	else {
		graph_init(&g, buf, _zwidth, _zheight);
		if(rotate == G_ROTATE_180) {
			gr = graph_new(NULL, _zwidth, _zheight);
		}
	}

	graph_t* tmp_g = &g;
	if(gr != NULL) {
		graph_rotate_to(&g, gr, rotate);
		tmp_g = gr;
	}

	if(_zoom > 0.0 && _zoom != 8.0 && _zoom != 1.0) {
		graph_t* gzoom = graph_new(NULL, fbinfo->width, fbinfo->height);
		graph_scale_tof(tmp_g, gzoom, _zoom);
		tmp_g = gzoom;
	}

	uint32_t res = _fbd->flush(fbinfo, tmp_g);
	if(gr != NULL)
		graph_free(gr);
	if(tmp_g != &g && tmp_g != gr)
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

static int fb_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
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
static int do_fb_flush(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;

	fb_dma_t* dma = (fb_dma_t*)p;
	return do_flush(dma);
}

static int32_t fb_dma(int fd, int from_pid, fsinfo_t* info, int* size, void* p) {
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


static int fb_dev_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
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
