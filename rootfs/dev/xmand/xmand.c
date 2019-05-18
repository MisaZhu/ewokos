#include <dev/devserv.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <graph/graph.h>
#include <graph/font.h>
#include <basic_math.h>
#include <kstring.h>
#include <vfs/fs.h>
#include <sconf.h>

typedef struct {
	uint32_t bg_color;
	uint32_t fg_color;
	font_t* font;

	fb_t fb;
	graph_t* graph;
	int32_t keyb_fd;
	int32_t mouse_fd;
	bool enabled;

	int32_t mx, my, mw, mh, mev;
	graph_t* mg;
} xman_t;

static xman_t _xman;
	
static int32_t read_config(void) {
	sconf_t *conf = sconf_load("/etc/xman.conf");	
	if(conf == NULL)
		return -1;
	
	const char* v = sconf_get(conf, "bg_color");
	if(v[0] != 0) 
		_xman.bg_color = rgb_int(atoi_base(v, 16));

	v = sconf_get(conf, "fg_color");
	if(v[0] != 0) 
		_xman.fg_color = rgb_int(atoi_base(v, 16));

	v = sconf_get(conf, "font");
	if(v[0] != 0) 
		_xman.font = get_font_by_name(v);

	sconf_free(conf, MFS);
	return 0;
}

static void xman_clear(void) {
	if(_xman.enabled)
		clear(_xman.graph, _xman.bg_color);
}

static int32_t xman_mount(const char* fname, int32_t index) {
	(void)fname;
	(void)index;
	_xman.enabled = false;

	_xman.bg_color = rgb(0x22, 0x22, 0x66);
	_xman.fg_color = rgb(0xaa, 0xbb, 0xaa);
	_xman.font = get_font_by_name("8x16");
	if(_xman.font == NULL)
		return -1;
	read_config();

	_xman.keyb_fd = open("/dev/keyb0", O_RDONLY);
	if(_xman.keyb_fd < 0)
		return -1;

	_xman.mouse_fd = open("/dev/mouse0", O_RDONLY);
	if(_xman.mouse_fd < 0) {
		close(_xman.keyb_fd);
		return -1;
	}

	if(fb_open("/dev/fb0", &_xman.fb) != 0)
		return -1;
	_xman.graph = graph_from_fb(&_xman.fb);

	_xman.mx=0;
	_xman.my=0;
	_xman.mw=16;
	_xman.mh=16;
	_xman.mg = NULL;
	return 0;
}

static int32_t xman_unmount(int32_t pid, const char* fname) {
	(void)pid;
	(void)fname;
	if(_xman.graph == NULL)
		return -1;
	close(_xman.keyb_fd);
	close(_xman.mouse_fd);
	graph_free(_xman.graph);
	graph_free(_xman.mg);
	_xman.graph = NULL;
	_xman.mg = NULL;
	fb_close(&_xman.fb);
	return 0;
}

static void flush(void) {
	if(_xman.enabled)
		fb_flush(&_xman.fb);
}

static void refresh(void) {
	xman_clear();
	flush();
}

static void xman_fctrl_raw(int32_t cmd, void* data, uint32_t size) {
	(void)data;
	(void)size;

	if(cmd == FS_CTRL_ENABLE) { //enable.
		_xman.enabled = true;
		refresh();
	}
	else if(cmd == FS_CTRL_DISABLE) { //disable.
		_xman.enabled = false;
	}
}

static void* xman_fctrl(int32_t pid, const char* fname, int32_t cmd, void* data, uint32_t size, int32_t* ret) {
	(void)pid;
	(void)fname;
	(void)ret;
	xman_fctrl_raw(cmd, data, size);
	return NULL;
}

static bool read_mouse(int32_t *x, int32_t *y, int32_t* ev, 
		int32_t mw, int32_t mh, uint32_t times) {
	int8_t mev[4];
	int sz = read(_xman.mouse_fd, mev, 4);
	if(sz <= 0)
		return false;
		
	*ev = mev[0];
	*x = *x + mev[1]*times;
	*y = *y + mev[2]*times;

	if(*x < -mw/2)
		*x = -mw/2;
	if(*x > (int32_t)_xman.graph->w - mw/2)
		*x = _xman.graph->w - mw/2;
	if(*y < -mh/2)
		*y = -mh/2;
	if(*y > (int32_t)_xman.graph->h - mh/2)
		*y = _xman.graph->h - mh/2;
	return true;
}

static inline void draw_cursor(int32_t mx, int32_t my, int32_t mw, int32_t mh) {
	line(_xman.graph, mx+1, my, mx+mw-1, my+mh-2, _xman.fg_color);
	line(_xman.graph, mx, my, mx+mw-1, my+mh-1, _xman.fg_color);
	line(_xman.graph, mx, my+1, mx+mw-2, my+mh-1, _xman.fg_color);

	line(_xman.graph, mx, my+mh-2, mx+mw-2, my, _xman.fg_color);
	line(_xman.graph, mx, my+mh-1, mx+mw-1, my, _xman.fg_color);
	line(_xman.graph, mx+1, my+mh-1, mx+mw-1, my+1, _xman.fg_color);
}

static void xman_step(void* p) {
	(void)p;
	if(!_xman.enabled) {
		sleep(0);
		return;
	}

	if(_xman.mg == NULL) {
		_xman.mg = graph_new(NULL, _xman.mw, _xman.mh);
		blt(_xman.graph, _xman.mx, _xman.my, _xman.mw, _xman.mh,
			  _xman.mg, 0, 0, _xman.mw, _xman.mh);	
	}

	blt(_xman.mg, 0, 0, _xman.mw, _xman.mh,
		  _xman.graph, _xman.mx, _xman.my, _xman.mw, _xman.mh);	

	if(read_mouse(&_xman.mx, &_xman.my, &_xman.mev, _xman.mw, _xman.mh, 2)) {
		blt(_xman.graph, _xman.mx, _xman.my, _xman.mw, _xman.mh,
			  _xman.mg, 0, 0, _xman.mw, _xman.mh);	
	}
	draw_cursor(_xman.mx, _xman.my, _xman.mw, _xman.mh);

	fb_flush(&_xman.fb);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.mount = xman_mount;
	dev.unmount = xman_unmount;
	dev.fctrl = xman_fctrl;
	dev.step = xman_step;

	dev_run(&dev, argc, argv);
	return 0;
}
