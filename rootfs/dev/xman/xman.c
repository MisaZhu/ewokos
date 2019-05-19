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
#include <x/xclient.h>

typedef struct {
	uint32_t bg_color;
	uint32_t fg_color;
	font_t* font;

	fb_t fb;
	graph_t* g;
	int32_t keyb_fd;
	int32_t mouse_fd;
	bool enabled;

	int32_t mx, my, mw, mh, mev;
	graph_t* mg;
	bool dirty;
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

static void xman_xclear(void) {
	if(_xman.enabled)
		clear(_xman.g, _xman.bg_color);
}

static void flush(void) {
	if(_xman.enabled)
		fb_flush(&_xman.fb);
}

typedef struct st_xres {
	graph_t* g;
	font_t* font;
	uint32_t fg_color;
	uint32_t bg_color;
	int32_t x, y;

	int32_t pid;
	int32_t fd;
	struct st_xres* next;
	struct st_xres* prev;
} xres_t;

static xres_t* _res_tail;

static void refresh(void) {
	xman_xclear();
	/*int32_t x, y;
	for(y=10; y<(int32_t)_xman.g->h; y+=10) {
		for(x=0; x<(int32_t)_xman.g->w; x+=10) {
			pixel(_xman.g, x, y, _xman.fg_color);
		}
	}
	*/
	draw_text(_xman.g, 10, 10, "EwokOS X Man", _xman.font, _xman.fg_color);

	xres_t* r = _res_tail;
	while(r != NULL && r->g != NULL) {
		blt(r->g, 0, 0, r->g->w, r->g->h,
			_xman.g, r->x, r->y, r->g->w, r->g->h);
		r = r->prev;
	}
	flush();
}

static int32_t xman_mount(const char* fname, int32_t index) {
	(void)fname;
	(void)index;
	_res_tail = NULL;

	_xman.enabled = true;
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
	_xman.g = graph_from_fb(&_xman.fb);

	_xman.mx=0;
	_xman.my=0;
	_xman.mw=14;
	_xman.mh=14;
	_xman.mg = NULL;
	_xman.dirty = 1;
	return 0;
}

static int32_t xman_unmount(int32_t pid, const char* fname) {
	(void)pid;
	(void)fname;
	if(_xman.g == NULL)
		return -1;
	close(_xman.keyb_fd);
	close(_xman.mouse_fd);
	graph_free(_xman.g);
	graph_free(_xman.mg);
	_xman.g = NULL;
	_xman.mg = NULL;
	fb_close(&_xman.fb);
	return 0;
}

static bool read_mouse(int32_t *x, int32_t *y, int32_t* ev, uint32_t times) {
	int8_t mev[4];
	int sz = read(_xman.mouse_fd, mev, 4);
	if(sz <= 0)
		return false;
		
	*ev = mev[0];
	*x = *x + mev[1]*times;
	*y = *y + mev[2]*times;

	if(*x < 0)
		*x = 0;
	if(*x > (int32_t)_xman.g->w)
		*x = _xman.g->w;
	if(*y < 0)
		*y = 0;
	if(*y > (int32_t)_xman.g->h)
		*y = _xman.g->h;
	return true;
}

static inline void draw_cursor(int32_t mx, int32_t my, int32_t mw, int32_t mh) {
	line(_xman.g, mx+1, my, mx+mw-1, my+mh-2, _xman.fg_color);
	line(_xman.g, mx, my, mx+mw-1, my+mh-1, _xman.fg_color);
	line(_xman.g, mx, my+1, mx+mw-2, my+mh-1, _xman.fg_color);

	line(_xman.g, mx, my+mh-2, mx+mw-2, my, _xman.fg_color);
	line(_xman.g, mx, my+mh-1, mx+mw-1, my, _xman.fg_color);
	line(_xman.g, mx+1, my+mh-1, mx+mw-1, my+1, _xman.fg_color);
}

static inline void xman_mouse(void) {
	if(_xman.mg == NULL) {
		_xman.mg = graph_new(NULL, _xman.mw, _xman.mh);
		blt(_xman.g, _xman.mx, _xman.my, _xman.mw, _xman.mh,
			  _xman.mg, 0, 0, _xman.mw, _xman.mh);	
	}

	blt(_xman.mg, 0, 0, _xman.mw, _xman.mh,
		  _xman.g, _xman.mx, _xman.my, _xman.mw, _xman.mh);	

	if(read_mouse(&_xman.mx, &_xman.my, &_xman.mev, 2)) {
		blt(_xman.g, _xman.mx, _xman.my, _xman.mw, _xman.mh,
			  _xman.mg, 0, 0, _xman.mw, _xman.mh);	
	}
	draw_cursor(_xman.mx, _xman.my, _xman.mw, _xman.mh);
}

static void xman_keyb(void) {
	char c;
	int32_t res = read(_xman.keyb_fd, &c, 1); 
	if(res == 1)
		printf("%c", c);	
}

static void xman_step(void* p) {
	(void)p;
	if(!_xman.enabled) {
		sleep(0);
		return;
	}

	if(_xman.dirty) {
		refresh();
		if(_xman.mg != NULL) {
			blt(_xman.g, _xman.mx, _xman.my, _xman.mw, _xman.mh,
				  _xman.mg, 0, 0, _xman.mw, _xman.mh);	
		}
		_xman.dirty = false;
	}

	xman_mouse();
	xman_keyb();

	fb_flush(&_xman.fb);
}

static int32_t xman_fctrl(int32_t pid, const char* fname, int32_t cmd, proto_t* input, proto_t* out) {
	(void)pid;
	(void)fname;
	(void)out;
	const char* data = proto_read_str(input);

	switch(cmd) {
	case FS_CTRL_SET_FONT: {//set font.
			font_t* fnt = get_font_by_name(data);
			if(fnt != NULL) {
				_xman.font = fnt;
				_xman.dirty = 1;
			}
		}
		return 0;
	case FS_CTRL_SET_FG_COLOR: //set fg color.
		_xman.fg_color = rgb_int(atoi_base(data, 16));
		_xman.dirty = 1;
		return 0;
	case FS_CTRL_SET_BG_COLOR: //set bg color.
		_xman.bg_color = rgb_int(atoi_base(data, 16));
		_xman.dirty = 1;
		return 0;
	case FS_CTRL_ENABLE: //enable.
		_xman.enabled = true;
		_xman.dirty = 1;
		return 0;
	case FS_CTRL_DISABLE: //disable.
		_xman.enabled = false;
		return 0;
	}
	return -1;
}

static xres_t* xman_get_res(int32_t pid, int32_t fd) {
	xres_t* r = _res_tail;
	while(r != NULL) {
		if(r->pid == pid && r->fd == fd)
			return r;
		r = r->prev;
	}
	return NULL;	
}

static xres_t* xman_get_top(void) {
	xres_t* r = _res_tail;
	while(r != NULL) {
		if(r->prev == NULL)
			return r;
		r = r->prev;
	}
	return NULL;	
}

static void xman_res_free(xres_t* res) {
	if(res->next != NULL)
		res->next->prev = res->prev;
	if(res->prev != NULL)
		res->prev->next = res->next;

	if(res == _res_tail)
		_res_tail = res->prev;
	
	graph_free(res->g);
	free(res);
}

static xres_t* xman_res_new(int32_t pid, int32_t fd) {
	xres_t* rtop = xman_get_top();

	xres_t* r = (xres_t*)malloc(sizeof(xres_t));
	memset(r, 0, sizeof(xres_t));
	r->g = graph_new(NULL, 400, 200);
	r->pid = pid;
	r->fd = fd;
	r->font = _xman.font;
	r->bg_color = _xman.bg_color;
	r->fg_color = _xman.fg_color;
	clear(r->g, r->bg_color);

	if(rtop == NULL)
		_res_tail = r;
	else {
		rtop->prev = r;
		r->next = rtop;
	}
	return r;
}

static int32_t xman_line(xres_t* res, proto_t* input) {
	line(res->g,
			proto_read_int(input),
			proto_read_int(input),
			proto_read_int(input),
			proto_read_int(input),
			res->fg_color);
	_xman.dirty = true;
	return 0;
}

static int32_t xman_box(xres_t* res, proto_t* input) {
	line(res->g,
			proto_read_int(input),
			proto_read_int(input),
			proto_read_int(input),
			proto_read_int(input),
			res->fg_color);
	_xman.dirty = true;
	return 0;
}

static int32_t xman_fill(xres_t* res, proto_t* input) {
	line(res->g,
			proto_read_int(input),
			proto_read_int(input),
			proto_read_int(input),
			proto_read_int(input),
			res->fg_color);
	_xman.dirty = true;
	return 0;
}

static int32_t xman_str(xres_t* res, proto_t* input) {
	draw_text(res->g,
			proto_read_int(input),
			proto_read_int(input),
			proto_read_str(input),
			res->font,
			res->fg_color);
	_xman.dirty = true;
	return 0;
}

static int32_t xman_move_to(xres_t* res, proto_t* input) {
	res->x = proto_read_int(input);
	res->y = proto_read_int(input);
	_xman.dirty = true;
	return 0;
}

static int32_t xman_resize_to(xres_t* res, proto_t* input) {
	int32_t w = proto_read_int(input);
	int32_t h = proto_read_int(input);
	graph_free(res->g);
	res->g = graph_new(NULL, w, h);
	_xman.dirty = true;
	return 0;
}

static int32_t xman_set_fg(xres_t* res, proto_t* input) {
	res->fg_color = (uint32_t)proto_read_int(input);
	return 0;
}

static int32_t xman_set_bg(xres_t* res, proto_t* input) {
	res->bg_color = (uint32_t)proto_read_int(input);
	return 0;
}

static int32_t xman_set_font(xres_t* res, proto_t* input) {
	font_t* font = get_font_by_name(proto_read_str(input));
	if(font != NULL)
		res->font = font;
	return 0;
}

static int32_t xman_clear(xres_t* res, proto_t* input) {
	(void)input;
	clear(res->g, res->bg_color);
	_xman.dirty = true;
	return 0;
}

static int32_t xman_ctrl(int32_t pid, int32_t fd, int32_t cmd, proto_t* input, proto_t* out) {
	(void)out;
	xres_t* res = xman_get_res(pid, fd);
	if(res == NULL)
		return -1;
	switch(cmd) {
	case X_CMD_MOVE_TO:
		return xman_move_to(res, input);
	case X_CMD_RESIZE_TO:
		return xman_resize_to(res, input);
	case X_CMD_LINE:
		return xman_line(res, input);
	case X_CMD_BOX:
		return xman_box(res, input);
	case X_CMD_FILL:
		return xman_fill(res, input);
	case X_CMD_STR:
		return xman_str(res, input);
	case X_CMD_SET_FG:
		return xman_set_fg(res, input);
	case X_CMD_SET_BG:
		return xman_set_bg(res, input);
	case X_CMD_SET_FONT:
		return xman_set_font(res, input);
	case X_CMD_CLEAR:
		return xman_clear(res, input);
	}
	return -1;
}

static int32_t xman_open(int32_t pid, int32_t fd, int32_t flags) {
	(void)flags;
	xres_t* res = xman_res_new(pid, fd);
	if(res == NULL)
		return -1;
	return 0;
}

static int32_t xman_close(int32_t pid, int32_t fd, fs_info_t* info) {
	(void)info;
	xres_t* res = xman_res_new(pid, fd);
	if(res == NULL)
		return -1;
	xman_res_free(res);
	return 0;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.mount = xman_mount;
	dev.unmount = xman_unmount;
	dev.fctrl = xman_fctrl;
	dev.ctrl = xman_ctrl;
	dev.open = xman_open;
	dev.close = xman_close;
	dev.step = xman_step;

	dev_run(&dev, argc, argv);
	return 0;
}
