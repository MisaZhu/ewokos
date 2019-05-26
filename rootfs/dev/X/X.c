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
#include <x/xwm.h>
#include <x/xclient.h>
#include <shm.h>
#include <ipc.h>

typedef struct st_xhandle {
	int32_t shm_id;
	void* g_buffer;

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
	int32_t mx, my;
	int32_t m_start_x, m_start_y;
	int32_t mw, mh, mev;
	graph_t* mg;
} cursor_t;

typedef struct {
	bool enabled;
	uint32_t bg_color;
	uint32_t fg_color;
	font_t* font;

	fb_t fb;
	graph_t* g;
	int32_t keyb_fd;
	int32_t cursor_fd;
	int32_t wm_pid;

	xhandle_t* mouse_owner;
	xhandle_t* handle_drag;
	xhandle_t* handle_top;
	xhandle_t* handle_bottom;

	cursor_t cursor;
	bool dirty;
	bool need_flush;
} xdev_t;

static xdev_t _X;

static void flush(void) {
	if(_X.enabled)
		fb_flush(&_X.fb);
}

static bool is_top(xhandle_t* r) {
	xhandle_t* h = _X.handle_top;
	while(h != NULL && h != r && h->state != X_STATE_HIDE) {
		return false;
		h = h->next;
	}
	return true;
}

static void draw_background(void){
	if(_X.wm_pid < 0) {
		clear(_X.g, 0);
		return;
	}
	ipc_call(_X.wm_pid, XWM_DRAW_BG, NULL, NULL);
}

static void draw_frame(xhandle_t* r) {
	if(_X.wm_pid < 0) 
		return;

	proto_t *in = proto_new(NULL, 0);
	if(is_top(r))
		proto_add_int(in, r->style | X_STYLE_TOP);
	else
		proto_add_int(in, r->style);

	proto_add_str(in, CS(r->title));
	proto_add_int(in, r->x-1);
	proto_add_int(in, r->y-1);
	proto_add_int(in, r->w+1);
	proto_add_int(in, r->h+1);

	ipc_call(_X.wm_pid, XWM_DRAW_FRAME, in, NULL);
	proto_free(in);
}

static void refresh(void) {
	if(_X.dirty) {
		draw_background();
		_X.need_flush = true;
	}
	//blt all handles
	xhandle_t* r = _X.handle_bottom;
	while(r != NULL && r->state != X_STATE_HIDE && r->g_buffer != NULL) {
		if(_X.dirty || r->dirty) {
			if(r->g_buffer != NULL) {
				graph_t* g = graph_new(r->g_buffer, r->w, r->h);
				if((r->style & X_STYLE_ALPHA) != 0)
					blt_alpha(g, 0, 0, r->w, r->h, _X.g, r->x, r->y, r->w, r->h, 0xff);
				else 
					blt(g, 0, 0, r->w, r->h, _X.g, r->x, r->y, r->w, r->h);
				graph_free(g);

				if((r->style & X_STYLE_NO_FRAME) == 0) {
					draw_frame(r);
				}
			}
			r->dirty = false;
			_X.need_flush = true;
		}
		r = r->prev;
	}
	_X.dirty = false;
}

static int32_t xserv_mount(const char* fname, int32_t index) {
	(void)fname;
	(void)index;
	_X.wm_pid = -1;
	_X.handle_drag = NULL;
	_X.mouse_owner = NULL;
	_X.handle_top = NULL;
	_X.handle_bottom = NULL;

	_X.enabled = true;
	_X.bg_color = argb_int(0xff222222);
	_X.fg_color = argb_int(0xffaaaaaa);
	_X.font = get_font_by_name("8x16");
	if(_X.font == NULL)
		return -1;

	_X.keyb_fd = open("/dev/keyb0", O_RDONLY);
	if(_X.keyb_fd < 0)
		return -1;

	_X.cursor_fd = open("/dev/mouse0", O_RDONLY);
	if(_X.cursor_fd < 0) {
		close(_X.keyb_fd);
		return -1;
	}

	if(fb_open("/dev/fb0", &_X.fb) != 0)
		return -1;
	_X.g = graph_from_fb(&_X.fb);

	_X.cursor.mx = _X.fb.w/2;
	_X.cursor.my = _X.fb.h/2;
	_X.cursor.mw = 14;
	_X.cursor.mh = 14;
	_X.cursor.offx = 7;
	_X.cursor.offy = 7;
	_X.cursor.mg = NULL;
	_X.dirty = 1;
	return 0;
}

static int32_t xserv_unmount(int32_t pid, const char* fname) {
	(void)pid;
	(void)fname;
	if(_X.g == NULL)
		return -1;
	close(_X.keyb_fd);
	close(_X.cursor_fd);
	graph_free(_X.g);
	graph_free(_X.cursor.mg);
	_X.g = NULL;
	_X.cursor.mg = NULL;
	fb_close(&_X.fb);
	return 0;
}

static bool read_mouse(int32_t *x, int32_t *y, int32_t* ev, int32_t times_perc) {
	int8_t mev[4];
	int sz = read(_X.cursor_fd, mev, 4);
	if(sz <= 0)
		return false;
		
	*ev = mev[0];
	*x = *x + mev[1]*times_perc/100;
	*y = *y + mev[2]*times_perc/100;

	if(*x < 0)
		*x = 0;
	if(*x > (int32_t)_X.g->w)
		*x = _X.g->w;
	if(*y < 0)
		*y = 0;
	if(*y > (int32_t)_X.g->h)
		*y = _X.g->h;
	return true;
}

static xhandle_t* get_mouse_owner(int32_t mx, int32_t my) {
	xhandle_t* h = _X.handle_top;
	while(h != NULL) {
		if(h->state != X_STATE_HIDE && 
				mx >= h->x && mx < (int32_t)(h->x+h->w) &&
				my >= h->y-20 && my < (int32_t)(h->y+h->h))
			return h;
		h = h->next;
	}
	return NULL;
}

static inline void handle_update(xhandle_t* handle) {
	if(handle == NULL)
		return;

	if((handle->style & X_STYLE_ALPHA) != 0) {
		_X.dirty = true;
		return;
	}

	xhandle_t* h = handle;
	while(h != NULL) {
		if(h->state != X_STATE_HIDE)
			h->dirty = true;
		h = h->prev;
	}
}

static void xdev_top(xhandle_t* handle) {
	if(handle == NULL || (handle->style & X_STYLE_NO_FRAME) != 0)
		return;
	if(_X.handle_top == NULL || handle == _X.handle_top) {
		_X.handle_top = handle;
		return;
	}
	handle_update(_X.handle_top);
	_X.handle_top->dirty = true;

	if(handle->next != NULL)
		handle->next->prev = handle->prev;
	if(handle->prev != NULL)
		handle->prev->next = handle->next;

	_X.handle_top->prev = handle;
	handle->next = _X.handle_top;

	if(handle == _X.handle_bottom) {
		_X.handle_bottom = handle->prev;
	}

	handle->prev = NULL;
	_X.handle_top = handle;
	_X.handle_top->dirty = true;
	_X.dirty = true;
}

static void xserv_event(xhandle_t* handle, x_ev_t* ev) {
	if(ev->type == X_EV_MOUSE) {
		if(ev->state == X_EV_MOUSE_DOWN) {
			if(ev->value.mouse.x > (handle->w-20) && ev->value.mouse.y < 0) { //check close click
				ev->type = X_EV_WIN;
				ev->state = X_EV_WIN_CLOSE;
			}
			else if(ev->value.mouse.x > 0 && ev->value.mouse.x < (handle->w-20) && ev->value.mouse.y < 0) { //check title mouse down
				_X.handle_drag = handle;
			}
			xdev_top(handle);
		}
	}
	x_ev_queue_push(&handle->events, ev, true);
}

static void xserv_mouse(void) {
	if(read_mouse(&_X.cursor.mx, &_X.cursor.my, &_X.cursor.mev, 140)) {
		int32_t mx = _X.cursor.mx + _X.cursor.offx;
		int32_t my = _X.cursor.my + _X.cursor.offy;
		_X.need_flush = true;
		x_ev_t ev;
		ev.type = X_EV_MOUSE;
		xhandle_t* owner = get_mouse_owner(mx, my);
		if(_X.cursor.mev == 0x0) {
			if(_X.handle_drag != NULL) { //drag 
				_X.handle_drag->x += mx - _X.cursor.m_start_x;
				_X.handle_drag->y += my - _X.cursor.m_start_y;
				_X.cursor.m_start_x = mx;
				_X.cursor.m_start_y = my;
				_X.dirty = true;
				return;
			}
			else {
				if(_X.mouse_owner != NULL) {
					ev.state = X_EV_MOUSE_DRAG;
					xserv_event(_X.mouse_owner, &ev);
				}
				return;
			}
		}
		if(_X.cursor.mev == 0x1) { //up
			_X.mouse_owner = NULL;
			_X.handle_drag = NULL;
		}

		if(owner != NULL) {
			ev.value.mouse.x = mx - owner->x;
			ev.value.mouse.y = my - owner->y;
	
			if(_X.cursor.mev == 0x2) { //down	
				ev.state = X_EV_MOUSE_DOWN;
				_X.cursor.m_start_x = mx;
				_X.cursor.m_start_y = my;
				_X.mouse_owner = owner;
			}
			else if(_X.cursor.mev == 0x0) {
				ev.state = X_EV_MOUSE_MOVE;
				xserv_event(owner, &ev);
			}
			else if(_X.cursor.mev == 0x1) {//up	
				ev.state = X_EV_MOUSE_UP;
				_X.mouse_owner = NULL;
				_X.handle_drag = NULL;
			}
			else {
					ev.state = X_EV_MOUSE_MOVE;
			}
			xserv_event(owner, &ev);
		}
	}
}

static xhandle_t* xserv_get_top_handle(void) {
	xhandle_t* r = _X.handle_top;
	while(r != NULL) {
		if(r->state != X_STATE_HIDE)
			return r;
		r = r->next;
	}
	return NULL;	
}

static void xserv_keyb(void) {
	char c;
	int32_t res = read(_X.keyb_fd, &c, 1); 
	xhandle_t* top = xserv_get_top_handle();
	if(res == 1 && top != NULL) {
		x_ev_t ev;
		ev.type = X_EV_KEYB;
		ev.value.keyboard.value = c;
		xserv_event(top, &ev);
	}
}

static inline void draw_cursor(void) {
	if(_X.cursor.mg == NULL)
		return;

	int32_t mx = _X.cursor.mx;
	int32_t my = _X.cursor.my;
	int32_t mw = _X.cursor.mw;
	int32_t mh = _X.cursor.mh;

	blt(_X.g, mx, my, mw, mh,
			_X.cursor.mg, 0, 0, mw, mh);	

	line(_X.g, mx+1, my, mx+mw-1, my+mh-2, _X.fg_color);
	line(_X.g, mx, my, mx+mw-1, my+mh-1, _X.fg_color);
	line(_X.g, mx, my+1, mx+mw-2, my+mh-1, _X.fg_color);

	line(_X.g, mx, my+mh-2, mx+mw-2, my, _X.fg_color);
	line(_X.g, mx, my+mh-1, mx+mw-1, my, _X.fg_color);
	line(_X.g, mx+1, my+mh-1, mx+mw-1, my+1, _X.fg_color);
}

static void hide_cursor(void) {
	if(_X.cursor.mg == NULL) {
		_X.cursor.mg = graph_new(NULL, _X.cursor.mw, _X.cursor.mh);
		blt(_X.g, _X.cursor.mx, _X.cursor.my, _X.cursor.mw, _X.cursor.mh,
			  _X.cursor.mg, 0, 0, _X.cursor.mw, _X.cursor.mh);	
	}
	else  {
		blt(_X.cursor.mg, 0, 0, _X.cursor.mw, _X.cursor.mh,
			  _X.g, _X.cursor.mx, _X.cursor.my, _X.cursor.mw, _X.cursor.mh);	
	}
}

static void xserv_step(void* p) {
	(void)p;
	if(!_X.enabled) {
		sleep(0);
		return;
	}
	hide_cursor();
	xserv_mouse();
	xserv_keyb();
	refresh();
	draw_cursor();

	if(_X.need_flush) {
		flush();
		_X.need_flush = false;
	}
}

static xhandle_t* xserv_get_handle(int32_t pid, int32_t fd) {
	xhandle_t* r = _X.handle_top;
	while(r != NULL) {
		if(r->pid == pid && r->fd == fd)
			return r;
		r = r->next;
	}
	return NULL;	
}

static void xserv_handle_free(xhandle_t* handle) {
	if(handle == NULL)
		return;

	if(handle->next != NULL)
		handle->next->prev = handle->prev;
	if(handle->prev != NULL)
		handle->prev->next = handle->next;

	if(handle == _X.handle_bottom)
		_X.handle_bottom = handle->prev;
	if(handle == _X.handle_top)
		_X.handle_top = handle->next;
	
	shm_free(handle->shm_id);
	tstr_free(handle->title);
	free(handle);
}

static xhandle_t* xserv_handle_new(int32_t pid, int32_t fd) {
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

	if(_X.handle_top == NULL) {
		_X.handle_bottom = r;
	}
	else {
		_X.handle_top->prev = r;
		r->next = _X.handle_top;
	}
	_X.handle_top = r;
	return r;
}

static int32_t xserv_move_to(xhandle_t* handle, proto_t* input) {
	handle->x = proto_read_int(input);
	handle->y = proto_read_int(input);
	_X.dirty = true;
	return 0;
}

static int32_t xserv_resize_to(xhandle_t* handle, proto_t* input) {
	int32_t w = proto_read_int(input);
	int32_t h = proto_read_int(input);
	handle->g_buffer = NULL;
	if(handle->shm_id >= 0)
		shm_free(handle->shm_id);
	handle->shm_id = shm_alloc(w*h*4);
	if(handle->shm_id < 0)
		return -1;
	handle->g_buffer = shm_map(handle->shm_id);
	handle->w = w;
	handle->h = h;
	handle_update(handle);
	_X.dirty = true;
	return 0;
}

static int32_t xserv_set_title(xhandle_t* handle, proto_t* input) {
	tstr_cpy(handle->title, proto_read_str(input));
	handle_update(handle);
	return 0;
}

static int32_t xserv_set_style(xhandle_t* handle, proto_t* input) {
	uint32_t style = proto_read_int(input);
	if(handle->style != style) {
		handle_update(handle);
		handle->style = style;
		handle->dirty = true;
		if((style & X_STYLE_NO_FRAME) != 0)	
			_X.dirty = true;
	}
	return 0;
}

static int32_t xserv_set_state(xhandle_t* handle, proto_t* input) {
	uint32_t state = proto_read_int(input);
	if(handle->state != state) {
		handle->state = state;
		_X.dirty = true;
	}
	return 0;
}

static int32_t xserv_get_event(xhandle_t* handle, proto_t* out) {
	x_ev_t ev;	
	if(x_ev_queue_pop(&handle->events, &ev) != 0)
		return -1;
	proto_add(out, &ev, sizeof(x_ev_t));
	return 0;
}

static int32_t xserv_get_scr_size(xhandle_t* handle, proto_t* out) {
	(void)handle;
	gsize_t sz = { _X.g->w, _X.g->h };
	proto_add(out, &sz, sizeof(gsize_t));
	return 0;
}

static int32_t xserv_flush(xhandle_t* handle) {
	if(handle == NULL)
		return -1;
	handle_update(handle);
	refresh();
	return 0;
}

static int32_t xserv_fctrl(int32_t pid, const char* fname, int32_t cmd, proto_t* input, proto_t* out) {
	(void)fname;
	(void)out;
	const char* data = proto_read_str(input);

	switch(cmd) {
	case FS_CTRL_SET_FONT: {//set font.
			font_t* fnt = get_font_by_name(data);
			if(fnt != NULL) {
				_X.font = fnt;
				_X.dirty = 1;
			}
		}
		return 0;
	case FS_CTRL_SET_FG_COLOR: //set fg color.
		_X.fg_color = argb_int(atoi_base(data, 16));
		_X.dirty = 1;
		return 0;
	case FS_CTRL_SET_BG_COLOR: //set bg color.
		_X.bg_color = argb_int(atoi_base(data, 16));
		_X.dirty = 1;
		return 0;
	case FS_CTRL_ENABLE: //enable.
		_X.enabled = true;
		_X.dirty = 1;
		return 0;
	case FS_CTRL_DISABLE: //disable.
		_X.enabled = false;
		return 0;
	case X_CMD_REG_WM: {//register window manager and return shm id (of framebuffer).
			proto_add_int(out, _X.fb.shm_id);
			proto_add_int(out, _X.fb.w);
			proto_add_int(out, _X.fb.h);
			_X.wm_pid = pid;
			_X.dirty = 1;
		}
		return 0;
	case X_CMD_FLUSH:
		flush();
		return 0;
	}
	return -1;
}

static int32_t xserv_ctrl(int32_t pid, int32_t fd, int32_t cmd, proto_t* input, proto_t* out) {
	xhandle_t* handle = xserv_get_handle(pid, fd);
	if(handle == NULL)
		return -1;
	switch(cmd) {
	case X_CMD_MOVE_TO:
		return xserv_move_to(handle, input);
	case X_CMD_RESIZE_TO:
		return xserv_resize_to(handle, input);
	case X_CMD_SET_TITLE:
		return xserv_set_title(handle, input);
	case X_CMD_SET_STYLE:
		return xserv_set_style(handle, input);
	case X_CMD_SET_STATE:
		return xserv_set_state(handle, input);
	case X_CMD_GET_EVENT:
		return xserv_get_event(handle, out);
	case X_CMD_GET_SCR_SIZE:
		return xserv_get_scr_size(handle, out);
	case X_CMD_FLUSH:
		return xserv_flush(handle);
	}
	return -1;
}

static int32_t xserv_open(int32_t pid, int32_t fd, int32_t flags) {
	(void)flags;
	xhandle_t* handle = xserv_handle_new(pid, fd);
	if(handle == NULL)
		return -1;
	return 0;
}

static int32_t xserv_dma(int32_t pid, int32_t fd, uint32_t *size) {
	xhandle_t* handle = xserv_get_handle(pid, fd);
	if(handle == NULL)
		return -1;

	*size = handle->w * handle->h *4;
	return handle->shm_id;
}

static int32_t xserv_close(int32_t pid, int32_t fd, fs_info_t* info) {
	(void)info;
	xhandle_t* handle = xserv_get_handle(pid, fd);
	if(handle == NULL)
		return -1;
	xserv_handle_free(handle);
	_X.dirty = 1;
	return 0;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.mount = xserv_mount;
	dev.unmount = xserv_unmount;
	dev.fctrl = xserv_fctrl;
	dev.ctrl = xserv_ctrl;
	dev.open = xserv_open;
	dev.close = xserv_close;
	dev.dma = xserv_dma;
	dev.step = xserv_step;

	dev_run(&dev, argc, argv);
	return 0;
}
