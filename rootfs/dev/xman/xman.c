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
#include <x/xcmd.h>
#include <x/xclient.h>
#include <shm.h>

typedef struct st_xhandle {
	int32_t shm_id;
	tstr_t* title;
	int32_t x, y, w, h;
	x_ev_queue_t events;
	uint32_t style;
	uint32_t state;
	bool dirty;

	int32_t pid;
	int32_t fd;
	struct st_xhandle* next;
	struct st_xhandle* prev;
} xhandle_t;

typedef struct {
	int32_t offx, offy;
	int32_t mx, my, mw, mh, mev;
	graph_t* mg;
} cursor_t;

typedef struct {
	uint32_t bg_color;
	uint32_t fg_color;
	font_t* font;

	fb_t fb;
	graph_t* g;
	int32_t keyb_fd;
	int32_t cursor_fd;
	bool enabled;

	xhandle_t* handle_top;
	xhandle_t* handle_bottom;

	cursor_t cursor;
	bool dirty;
	bool need_flush;
} xman_t;

static xman_t _xman;

static void xman_xclear(void) {
	if(_xman.enabled)
		clear(_xman.g, _xman.bg_color);
}

static void flush(void) {
	if(_xman.enabled)
		fb_flush(&_xman.fb);
}

static void draw_title(xhandle_t* r) {
	uint32_t fill_color = _xman.bg_color;
	uint32_t fg_color = _xman.fg_color;
	uint32_t text_color = _xman.fg_color;

	if(r == _xman.handle_top) {
		fill_color = 0xaaaaaa;
		text_color = 0x0;
	}

	fill(_xman.g, r->x, r->y-20, r->w, 20, fill_color);//title box
	box(_xman.g, r->x, r->y-20, r->w, 20, fg_color);//title box
	box(_xman.g, r->x+r->w-20, r->y-20, 20, 20, fg_color);//close box
	draw_text(_xman.g, r->x, r->y-20, CS(r->title), _xman.font, text_color);//title
}

static void refresh(void) {
	if(_xman.dirty) {
		xman_xclear();
		//background pattern
		int32_t x, y;
		for(y=20; y<(int32_t)_xman.g->h; y+=20) {
			for(x=0; x<(int32_t)_xman.g->w; x+=20) {
				pixel(_xman.g, x, y, _xman.fg_color);
			}
		}
		_xman.need_flush = true;
	}

	//blt all handles
	xhandle_t* r = _xman.handle_bottom;
	while(r != NULL && r->state != X_STATE_HIDE && r->shm_id >= 0) {
		if(_xman.dirty || r->dirty) {
			void *p = shm_map(r->shm_id);
			if(p != NULL) {
				graph_t* g = graph_new(p, r->w, r->h);
				blt(g, 0, 0, r->w, r->h, _xman.g, r->x, r->y, r->w, r->h);
				graph_free(g);

				if((r->style & X_STYLE_NO_FRAME) == 0) {
					box(_xman.g, r->x, r->y, r->w, r->h, _xman.fg_color);//win box
					draw_title(r);
				}
			}
			r->dirty = false;
			_xman.need_flush = true;
		}
		r = r->prev;
	}
	_xman.dirty = false;
}

static int32_t xman_mount(const char* fname, int32_t index) {
	(void)fname;
	(void)index;
	_xman.handle_top = NULL;
	_xman.handle_top = NULL;
	_xman.handle_bottom = NULL;

	_xman.enabled = true;
	_xman.bg_color = rgb_int(0x222222);
	_xman.fg_color = rgb_int(0xaaaaaa);
	_xman.font = get_font_by_name("8x16");
	if(_xman.font == NULL)
		return -1;

	_xman.keyb_fd = open("/dev/keyb0", O_RDONLY);
	if(_xman.keyb_fd < 0)
		return -1;

	_xman.cursor_fd = open("/dev/mouse0", O_RDONLY);
	if(_xman.cursor_fd < 0) {
		close(_xman.keyb_fd);
		return -1;
	}

	if(fb_open("/dev/fb0", &_xman.fb) != 0)
		return -1;
	_xman.g = graph_from_fb(&_xman.fb);

	_xman.cursor.mx = _xman.fb.w/2;
	_xman.cursor.my = _xman.fb.h/2;
	_xman.cursor.mw = 14;
	_xman.cursor.mh = 14;
	_xman.cursor.offx = 7;
	_xman.cursor.offy = 7;
	_xman.cursor.mg = NULL;
	_xman.dirty = 1;
	return 0;
}

static int32_t xman_unmount(int32_t pid, const char* fname) {
	(void)pid;
	(void)fname;
	if(_xman.g == NULL)
		return -1;
	close(_xman.keyb_fd);
	close(_xman.cursor_fd);
	graph_free(_xman.g);
	graph_free(_xman.cursor.mg);
	_xman.g = NULL;
	_xman.cursor.mg = NULL;
	fb_close(&_xman.fb);
	return 0;
}

static bool read_mouse(int32_t *x, int32_t *y, int32_t* ev, uint32_t times) {
	int8_t mev[4];
	int sz = read(_xman.cursor_fd, mev, 4);
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

static xhandle_t* get_mouse_owner(int32_t mx, int32_t my) {
	xhandle_t* h = _xman.handle_top;
	while(h != NULL) {
		if(h->state != X_STATE_HIDE && 
				mx >= h->x && mx < (int32_t)(h->x+h->w) &&
				my >= h->y-20 && my < (int32_t)(h->y+h->h))
			return h;
		h = h->next;
	}
	return NULL;
}

static void xman_top(xhandle_t* handle) {
	if(handle == NULL)
		return;
	if(_xman.handle_top == NULL || handle == _xman.handle_top) {
		_xman.handle_top = handle;
		return;
	}

	if(handle->next != NULL)
		handle->next->prev = handle->prev;
	if(handle->prev != NULL)
		handle->prev->next = handle->next;

	_xman.handle_top->prev = handle;
	handle->next = _xman.handle_top;

	if(handle == _xman.handle_bottom) {
		_xman.handle_bottom = handle->prev;
	}

	handle->prev = NULL;
	_xman.handle_top = handle;
	_xman.dirty = true;
}

static void xman_event(xhandle_t* handle, x_ev_t* ev) {
	if(ev->type == X_EV_MOUSE) {
		if(ev->state == X_EV_MOUSE_DOWN) {
			if(ev->value.mouse.x > (handle->w-20) && ev->value.mouse.y < 0) { //check close click
				ev->type = X_EV_WIN;
				ev->state = X_EV_WIN_CLOSE;
			}
			else {
				xman_top(handle);
			}
		}
	}
	x_ev_queue_push(&handle->events, ev, true);
}

static void xman_mouse(void) {
	if(read_mouse(&_xman.cursor.mx, &_xman.cursor.my, &_xman.cursor.mev, 1)) {
		_xman.need_flush = true;
		xhandle_t* owner = get_mouse_owner(_xman.cursor.mx, _xman.cursor.my);
		if(owner != NULL) {
			x_ev_t ev;
			ev.type = X_EV_MOUSE;
			ev.value.mouse.x = _xman.cursor.mx - owner->x + _xman.cursor.offx;
			ev.value.mouse.y = _xman.cursor.my - owner->y + _xman.cursor.offy;
			if(_xman.cursor.mev == 0x2) //down	
				ev.state = X_EV_MOUSE_DOWN;
			else if(_xman.cursor.mev == 0x1) //up	
				ev.state = X_EV_MOUSE_UP;
			else
				ev.state = X_EV_MOUSE_MOVE;
			xman_event(owner, &ev);
		}
	}
}

static xhandle_t* xman_get_top_handle(void) {
	xhandle_t* r = _xman.handle_top;
	while(r != NULL) {
		if(r->state != X_STATE_HIDE)
			return r;
		r = r->next;
	}
	return NULL;	
}

static void xman_keyb(void) {
	char c;
	int32_t res = read(_xman.keyb_fd, &c, 1); 
	xhandle_t* top = xman_get_top_handle();
	if(res == 1 && top != NULL) {
		x_ev_t ev;
		ev.type = X_EV_KEYB;
		ev.value.keyboard.value = c;
		xman_event(top, &ev);
	}
}

static inline void draw_cursor(void) {
	if(_xman.cursor.mg == NULL)
		return;

	int32_t mx = _xman.cursor.mx;
	int32_t my = _xman.cursor.my;
	int32_t mw = _xman.cursor.mw;
	int32_t mh = _xman.cursor.mh;

	blt(_xman.g, mx, my, mw, mh,
			_xman.cursor.mg, 0, 0, mw, mh);	

	line(_xman.g, mx+1, my, mx+mw-1, my+mh-2, _xman.fg_color);
	line(_xman.g, mx, my, mx+mw-1, my+mh-1, _xman.fg_color);
	line(_xman.g, mx, my+1, mx+mw-2, my+mh-1, _xman.fg_color);

	line(_xman.g, mx, my+mh-2, mx+mw-2, my, _xman.fg_color);
	line(_xman.g, mx, my+mh-1, mx+mw-1, my, _xman.fg_color);
	line(_xman.g, mx+1, my+mh-1, mx+mw-1, my+1, _xman.fg_color);
}

static void hide_cursor(void) {
	if(_xman.cursor.mg == NULL) {
		_xman.cursor.mg = graph_new(NULL, _xman.cursor.mw, _xman.cursor.mh);
		blt(_xman.g, _xman.cursor.mx, _xman.cursor.my, _xman.cursor.mw, _xman.cursor.mh,
			  _xman.cursor.mg, 0, 0, _xman.cursor.mw, _xman.cursor.mh);	
	}
	else  {
		blt(_xman.cursor.mg, 0, 0, _xman.cursor.mw, _xman.cursor.mh,
			  _xman.g, _xman.cursor.mx, _xman.cursor.my, _xman.cursor.mw, _xman.cursor.mh);	
	}
}

static void xman_step(void* p) {
	(void)p;
	if(!_xman.enabled) {
		sleep(0);
		return;
	}
	hide_cursor();
	refresh();
	xman_mouse();
	xman_keyb();
	draw_cursor();

	if(_xman.need_flush) {
		flush();
		_xman.need_flush = false;
	}
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

static inline void handle_update(xhandle_t* handle) {
	if(handle == NULL)
		return;
	handle->dirty = true;
	if(handle != _xman.handle_top && handle->state != X_STATE_HIDE)
		_xman.dirty = true;
}

static xhandle_t* xman_get_handle(int32_t pid, int32_t fd) {
	xhandle_t* r = _xman.handle_top;
	while(r != NULL) {
		if(r->pid == pid && r->fd == fd)
			return r;
		r = r->next;
	}
	return NULL;	
}

static void xman_handle_free(xhandle_t* handle) {
	if(handle->next != NULL)
		handle->next->prev = handle->prev;
	if(handle->prev != NULL)
		handle->prev->next = handle->next;

	if(handle == _xman.handle_bottom)
		_xman.handle_bottom = handle->prev;
	if(handle == _xman.handle_top)
		_xman.handle_top = handle->next;
	
	shm_unmap(handle->shm_id);
	tstr_free(handle->title);
	free(handle);
}

static xhandle_t* xman_handle_new(int32_t pid, int32_t fd) {
	xhandle_t* r = (xhandle_t*)malloc(sizeof(xhandle_t));
	memset(r, 0, sizeof(xhandle_t));
	r->shm_id = -1;
	r->x = 0;
	r->y = 0;
	r->w = 0;
	r->h = 0;
	r->title = tstr_new("", MFS);
	r->pid = pid;
	r->fd = fd;
	x_ev_queue_init(&r->events);

	if(_xman.handle_top == NULL) {
		_xman.handle_bottom = r;
	}
	else {
		_xman.handle_top->prev = r;
		r->next = _xman.handle_top;
	}
	_xman.handle_top = r;
	return r;
}

static int32_t xman_move_to(xhandle_t* handle, proto_t* input) {
	handle->x = proto_read_int(input);
	handle->y = proto_read_int(input);
	_xman.dirty = true;
	return 0;
}

static int32_t xman_resize_to(xhandle_t* handle, proto_t* input) {
	int32_t w = proto_read_int(input);
	int32_t h = proto_read_int(input);
	if(handle->shm_id >= 0)
		shm_unmap(handle->shm_id);
	handle->shm_id = shm_alloc(w*h*4);
	if(handle->shm_id < 0)
		return -1;
	handle->w = w;
	handle->h = h;
	handle_update(handle);
	_xman.dirty = true;
	return 0;
}

static int32_t xman_set_title(xhandle_t* handle, proto_t* input) {
	tstr_cpy(handle->title, proto_read_str(input));
	handle_update(handle);
	return 0;
}

static int32_t xman_set_style(xhandle_t* handle, proto_t* input) {
	uint32_t style = proto_read_int(input);
	if(handle->style != style) {
		handle_update(handle);
		handle->style = style;
		handle->dirty = true;
		if((style & X_STYLE_NO_FRAME) != 0)	
			_xman.dirty = true;
	}
	return 0;
}

static int32_t xman_set_state(xhandle_t* handle, proto_t* input) {
	uint32_t state = proto_read_int(input);
	if(handle->state != state) {
		handle->state = state;
		_xman.dirty = true;
	}
	return 0;
}

static int32_t xman_get_event(xhandle_t* handle, proto_t* out) {
	x_ev_t ev;	
	if(x_ev_queue_pop(&handle->events, &ev) != 0)
		return -1;
	proto_add(out, &ev, sizeof(x_ev_t));
	return 0;
}

static int32_t xman_get_scr_size(xhandle_t* handle, proto_t* out) {
	(void)handle;
	gsize_t sz = { _xman.g->w, _xman.g->h };
	proto_add(out, &sz, sizeof(gsize_t));
	return 0;
}

static int32_t xman_flush(xhandle_t* handle) {
	if(handle == NULL)
		return -1;
	handle_update(handle);
	refresh();
	return 0;
}

static int32_t xman_ctrl(int32_t pid, int32_t fd, int32_t cmd, proto_t* input, proto_t* out) {
	xhandle_t* handle = xman_get_handle(pid, fd);
	if(handle == NULL)
		return -1;
	switch(cmd) {
	case X_CMD_MOVE_TO:
		return xman_move_to(handle, input);
	case X_CMD_RESIZE_TO:
		return xman_resize_to(handle, input);
	case X_CMD_SET_TITLE:
		return xman_set_title(handle, input);
	case X_CMD_SET_STYLE:
		return xman_set_style(handle, input);
	case X_CMD_SET_STATE:
		return xman_set_state(handle, input);
	case X_CMD_GET_EVENT:
		return xman_get_event(handle, out);
	case X_CMD_GET_SCR_SIZE:
		return xman_get_scr_size(handle, out);
	case X_CMD_FLUSH:
		return xman_flush(handle);
	}
	return -1;
}

static int32_t xman_open(int32_t pid, int32_t fd, int32_t flags) {
	(void)flags;
	xhandle_t* handle = xman_handle_new(pid, fd);
	if(handle == NULL)
		return -1;
	return 0;
}

static int32_t xman_dma(int32_t pid, int32_t fd, uint32_t *size) {
	xhandle_t* handle = xman_get_handle(pid, fd);
	if(handle == NULL)
		return -1;

	*size = handle->w * handle->h *4;
	return handle->shm_id;
}

static int32_t xman_close(int32_t pid, int32_t fd, fs_info_t* info) {
	(void)info;
	xhandle_t* handle = xman_get_handle(pid, fd);
	if(handle == NULL)
		return -1;
	xman_handle_free(handle);
	_xman.dirty = 1;
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
	dev.dma = xman_dma;
	dev.step = xman_step;

	dev_run(&dev, argc, argv);
	return 0;
}
