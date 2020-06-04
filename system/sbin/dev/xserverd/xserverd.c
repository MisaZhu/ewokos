#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/lockc.h>
#include <sys/syscall.h>
#include <sys/shm.h>
#include <graph/graph.h>
#include <fbinfo.h>
#include <sys/ipc.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwm.h>
#include <sys/global.h>
#include <pthread.h>
#include <sys/proc.h>
#include <sconf.h>

#define X_EVENT_MAX 16

typedef struct st_xview {
	int fd;
	int from_pid;
	uint32_t ufid;
	graph_t* g;
	xinfo_t xinfo;
	bool dirty;

	struct st_xview *next;
	struct st_xview *prev;
} xview_t;

typedef struct {
	gpos_t old_pos;
	gpos_t cpos;
	gpos_t offset;
	gsize_t size;
	graph_t* g;
	bool drop;
} cursor_t;

typedef struct {
	xview_t* view; //moving or resizing;
	gpos_t old_pos;
} x_current_t;

typedef struct {
	int win_move_alpha;
	char xwm[128];
} x_conf_t;

typedef struct {
	bool actived;
	int fb_fd;
	int keyb_fd;
	int mouse_fd;
	int joystick_fd;
	int xwm_pid;
	int shm_id;
	bool dirty;
	bool need_repaint;
	bool show_cursor;
	graph_t* g;
	cursor_t cursor;

	xview_t* view_head;
	xview_t* view_tail;

	x_current_t current;
	x_conf_t config;

	uint32_t lock;
} x_t;

static int32_t read_config(x_t* x, const char* fname) {
	x->config.win_move_alpha = 0x88;
	x->config.xwm[0] = 0;

	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL)
		return -1;

	const char* v = sconf_get(conf, "win_move_alpha");
	if(v[0] != 0) 
		x->config.win_move_alpha = atoi_base(v, 16);

	v = sconf_get(conf, "xwm");
	if(v[0] != 0) 
		strncpy(x->config.xwm, v, 127);

	sconf_free(conf);
	return 0;
}


static void draw_win_frame(x_t* x, xview_t* view) {
	if((view->xinfo.style & X_STYLE_NO_FRAME) != 0)
		return;

	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		addi(&in, x->shm_id)->
		addi(&in, x->g->w)->
		addi(&in, x->g->h)->
		add(&in, &view->xinfo, sizeof(xinfo_t));
	if(view == x->view_tail)
		PF->addi(&in, 1); //top win
	else
		PF->addi(&in, 0);

	ipc_call(x->xwm_pid, XWM_CNTL_DRAW_FRAME, &in, &out);
	PF->clear(&in);
	PF->clear(&out);
}

static void draw_desktop(x_t* x) {
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		addi(&in, x->shm_id)->
		addi(&in, x->g->w)->
		addi(&in, x->g->h);

	ipc_call(x->xwm_pid, XWM_CNTL_DRAW_DESKTOP, &in, &out);
	PF->clear(&in);
	PF->clear(&out);
}

static int draw_view(x_t* xp, xview_t* view) {
	if(!xp->dirty && !view->dirty)
		return 0;

	if(view->g != NULL) {
		if(xp->current.view == view && xp->config.win_move_alpha < 0xff) { //drag and moving
			blt_alpha(view->g, 0, 0, 
					view->xinfo.wsr.w,
					view->xinfo.wsr.h,
					xp->g,
					view->xinfo.wsr.x,
					view->xinfo.wsr.y,
					view->xinfo.wsr.w,
					view->xinfo.wsr.h, xp->config.win_move_alpha);
		}
		else	if((view->xinfo.style & X_STYLE_ALPHA) != 0) {
			blt_alpha(view->g, 0, 0, 
					view->xinfo.wsr.w,
					view->xinfo.wsr.h,
					xp->g,
					view->xinfo.wsr.x,
					view->xinfo.wsr.y,
					view->xinfo.wsr.w,
					view->xinfo.wsr.h, 0xff);
		}
		else {
			blt(view->g, 0, 0, 
						view->xinfo.wsr.w,
						view->xinfo.wsr.h,
						xp->g,
						view->xinfo.wsr.x,
						view->xinfo.wsr.y,
						view->xinfo.wsr.w,
						view->xinfo.wsr.h);
		}
	}

	draw_win_frame(xp, view);
	view->dirty = false;
	return 0;
}

static inline void x_dirty(x_t* x) {
	x->dirty = true;
	x->need_repaint = true;
}

static void remove_view(x_t* x, xview_t* view) {
	if(view->prev != NULL)
		view->prev->next = view->next;
	if(view->next != NULL)
		view->next->prev = view->prev;
	if(x->view_tail == view)
		x->view_tail = view->prev;
	if(x->view_head == view)
		x->view_head = view->next;
	view->next = view->prev = NULL;
	x_dirty(x);
}

static void x_push_event(xview_t* view, xevent_t* e) {
	if(view->from_pid <= 0)
		return;
	proto_t in;
	PF->init(&in, NULL, 0)->add(&in, e, sizeof(xevent_t));
	ipc_call(view->from_pid, X_CMD_PUSH_EVENT, &in, NULL);
	PF->clear(&in);
}

static void push_view(x_t* x, xview_t* view) {
	if((view->xinfo.style & X_STYLE_NO_FOCUS) == 0) {
		if(x->view_tail != NULL) {
			xevent_t e;
			e.type = XEVT_WIN;
			e.value.window.event = XEVT_WIN_UNFOCUS;
			x_push_event(x->view_tail, &e);

			x->view_tail->next = view;
			view->prev = x->view_tail;
			x->view_tail = view;
		}
		else {
			x->view_tail = x->view_head = view;
		}

		xevent_t e;
		e.type = XEVT_WIN;
		e.value.window.event = XEVT_WIN_FOCUS;
		x_push_event(view, &e);
	}
	else {
		if(x->view_head != NULL) {
			x->view_head->prev = view;
			view->next = x->view_head;
			x->view_head = view;
		}
		else {
			x->view_tail = x->view_head = view;
		}
	}

	if(view->xinfo.visible)
		x_dirty(x);
}

static void x_del_view(x_t* x, xview_t* view) {
	remove_view(x, view);
	free(view);

	if(x->view_tail != NULL) {
		xevent_t e;
		e.type = XEVT_WIN;
		e.value.window.event = XEVT_WIN_FOCUS;
		x_push_event(x->view_tail, &e);
	}
}

static void hide_cursor(x_t* x) {
	if(x->cursor.drop)
		return;

	if(x->cursor.g == NULL) {
		x->cursor.g = graph_new(NULL, x->cursor.size.w, x->cursor.size.h);
		blt(x->g,
				x->cursor.old_pos.x - x->cursor.offset.x,
				x->cursor.old_pos.y - x->cursor.offset.y,
				x->cursor.size.w,
				x->cursor.size.h,
				x->cursor.g,
				0,
				0, 
				x->cursor.size.w,
				x->cursor.size.h);
	}
	else  {
		blt(x->cursor.g,
				0,
				0,
				x->cursor.size.w,
				x->cursor.size.h,
				x->g,
				x->cursor.old_pos.x - x->cursor.offset.x,
				x->cursor.old_pos.y - x->cursor.offset.x,
				x->cursor.size.w,
				x->cursor.size.h);
	}
}

static inline void draw_cursor(x_t* x) {
	int32_t mx = x->cursor.cpos.x - x->cursor.offset.x;
	int32_t my = x->cursor.cpos.y - x->cursor.offset.y;
	int32_t mw = x->cursor.size.w;
	int32_t mh = x->cursor.size.h;

	if(x->cursor.g == NULL)
		return;

	blt(x->g, mx, my, mw, mh,
			x->cursor.g, 0, 0, mw, mh);

	line(x->g, mx+1, my, mx+mw-1, my+mh-2, 0xffffffff);
	line(x->g, mx, my, mx+mw-1, my+mh-1, 0xff000000);
	line(x->g, mx, my+1, mx+mw-2, my+mh-1, 0xffffffff);

	line(x->g, mx, my+mh-2, mx+mw-2, my, 0xffffffff);
	line(x->g, mx, my+mh-1, mx+mw-1, my, 0xff000000);
	line(x->g, mx+1, my+mh-1, mx+mw-1, my+1, 0xffffffff);
	x->cursor.old_pos.x = x->cursor.cpos.x;
	x->cursor.old_pos.y = x->cursor.cpos.y;
	x->cursor.drop = false;
}

static void x_repaint(x_t* x) {
	if(!x->actived ||
			(!x->need_repaint))
		return;
	x->need_repaint = false;

	hide_cursor(x);
	bool undirty = false;
	if(x->dirty) {
		draw_desktop(x);
		undirty = true;
	}

	xview_t* view = x->view_head;
	while(view != NULL) {
		if(view->xinfo.visible)
			draw_view(x, view);
		view = view->next;
	}
	if(x->show_cursor)
		draw_cursor(x);
	flush(x->fb_fd);

	if(undirty) {
		x->dirty = false;
	}
}

static xview_t* x_get_view(x_t* x, int ufid, int from_pid) {
	xview_t* view = x->view_head;
	while(view != NULL) {
		if(view->ufid == (uint32_t)ufid && view->from_pid == from_pid)
			return view;
		view = view->next;
	}
	return NULL;
}

static int x_update(int ufid, int from_pid, x_t* x) {
	if(ufid < 0)
		return -1;
	
	xview_t* view = x_get_view(x, ufid, from_pid);
	if(view == NULL)
		return -1;

	lock_lock(x->lock);
	view->dirty = true;
	if(view != x->view_tail ||
			(view->xinfo.style & X_STYLE_ALPHA) != 0) {
		x_dirty(x);
	}
	x->need_repaint = true;
	lock_unlock(x->lock);
	return 0;
}

static int x_set_visible(int ufid, int from_pid, proto_t* in, x_t* x) {
	if(ufid < 0)
		return -1;
	
	xview_t* view = x_get_view(x, ufid, from_pid);
	if(view == NULL)
		return -1;

	view->xinfo.visible = proto_read_int(in);
	view->dirty = true;
	x_dirty(x);
	return 0;
}

static int x_update_info(int ufid, int from_pid, proto_t* in, x_t* x) {
	xinfo_t xinfo;
	int sz = sizeof(xinfo_t);
	if(ufid == 0 || proto_read_to(in, &xinfo, sz) != sz)
		return -1;
	
	xview_t* view = x_get_view(x, ufid, from_pid);
	if(view == NULL)
		return -1;
	
	int shm_id = view->xinfo.shm_id;
	if(shm_id == 0 ||
			view->g == NULL ||
			view->xinfo.wsr.w != xinfo.wsr.w ||
			view->xinfo.wsr.h != xinfo.wsr.h) {
		if(view->g != NULL && shm_id > 0) {
			graph_free(view->g);
			shm_unmap(shm_id);
		}
		shm_id = shm_alloc(xinfo.wsr.w * xinfo.wsr.h * 4, 1);
		void* p = shm_map(shm_id);
		if(p == NULL) 
			return -1;
		view->g = graph_new(p, xinfo.wsr.w, xinfo.wsr.h);
		clear(view->g, 0xff000000);
	}
	view->dirty = true;
	x->need_repaint = true;

	if(view != x->view_tail ||
			view->xinfo.wsr.x != xinfo.wsr.x ||
			view->xinfo.wsr.y != xinfo.wsr.y ||
			view->xinfo.wsr.w != xinfo.wsr.w ||
			view->xinfo.wsr.h != xinfo.wsr.h ||
			view->xinfo.visible != xinfo.visible ||
			(view->xinfo.style & X_STYLE_ALPHA) != 0) {
		x_dirty(x);
	}
	memcpy(&view->xinfo, &xinfo, sizeof(xinfo_t));
	view->xinfo.shm_id = shm_id;

	return 0;
}

static int x_get_info(int ufid, int from_pid, x_t* x, proto_t* out) {
	xview_t* view = x_get_view(x, ufid, from_pid);
	if(view == NULL)
		return -1;
	PF->add(out, &view->xinfo, sizeof(xinfo_t));
	return 0;
}

static int x_is_top(int ufid, int from_pid, x_t* x, proto_t* out) {
	xview_t* view = x_get_view(x, ufid, from_pid);
	if(view == NULL || x->view_tail == NULL)
		return -1;

	if(view == x->view_tail)
		PF->addi(out, 0);
	else
		PF->addi(out, -1);
	return 0;
}

static int get_xwm_workspace(x_t* x, int style, grect_t* rin, grect_t* rout) {
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		addi(&in, style)->
		add(&in, rin, sizeof(grect_t));

	ipc_call(x->xwm_pid, XWM_CNTL_GET_WORKSPACE, &in, &out);

	PF->clear(&in);
	proto_read_to(&out, rout, sizeof(grect_t));
	PF->clear(&out);
	return 0;
}

static int x_workspace(x_t* x, proto_t* in, proto_t* out) {
	grect_t r;
	int style = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));
	get_xwm_workspace(x, style, &r, &r);
	PF->add(out, &r, sizeof(grect_t));
	return 0;
}

static int xserver_fcntl(int fd, int ufid, int from_pid, fsinfo_t* info,
		int cmd, proto_t* in, proto_t* out, void* p) {
	(void)fd;
	(void)info;
	x_t* x = (x_t*)p;

	if(cmd == X_CNTL_UPDATE) {
		return x_update(ufid, from_pid, x);
	}	
	else if(cmd == X_CNTL_UPDATE_INFO) {
		return x_update_info(ufid, from_pid, in, x);
	}
	else if(cmd == X_CNTL_SET_VISIBLE) {
		return x_set_visible(ufid, from_pid, in, x);
	}
	else if(cmd == X_CNTL_GET_INFO) {
		return x_get_info(ufid, from_pid, x, out);
	}
	else if(cmd == X_CNTL_WORKSPACE) {
		return x_workspace(x, in, out);
	}
	else if(cmd == X_CNTL_IS_TOP) {
		return x_is_top(ufid, from_pid, x, out);
	}

	return 0;
}

static int xserver_open(int fd, int ufid, int from_pid, fsinfo_t* info, int oflag, void* p) {
	(void)oflag;
	(void)info;
	if(fd < 0 || ufid == 0)
		return -1;

	x_t* x = (x_t*)p;
	xview_t* view = (xview_t*)malloc(sizeof(xview_t));
	if(view == NULL)
		return -1;

	lock_lock(x->lock);
	memset(view, 0, sizeof(xview_t));
	view->fd = fd;
	view->ufid = ufid;
	view->from_pid = from_pid;
	push_view(x, view);
	lock_unlock(x->lock);
	return 0;
}

static int xserver_info(int from_pid, const char* fname, fsinfo_t* info, proto_t* ret, void* p) {
	(void)from_pid;
	(void)fname;
	(void)info;
	x_t* x = (x_t*)p;

	xscreen_t scr;	
	scr.id = 0;
	scr.size.w = x->g->w;
	scr.size.h = x->g->h;
	PF->add(ret, &scr, sizeof(xscreen_t));
	return 0;
}

static int xserver_close(int fd, int ufid, int from_pid, fsinfo_t* info, void* p) {
	(void)info;
	(void)fd;
	x_t* x = (x_t*)p;
	xview_t* view = x_get_view(x, ufid, from_pid);
	if(view == NULL) {
		return -1;
	}
	lock_lock(x->lock);
	x_del_view(x, view);	
	lock_unlock(x->lock);
	return 0;
}

static void x_check_views(x_t* x) {
	xview_t* view = x->view_head;
	lock_lock(x->lock);
	while(view != NULL) {
		xview_t* next = view->next;
		uint32_t ufid = vfs_check_fd(view->from_pid, view->fd);
		if(ufid == 0 || ufid != view->ufid)
			x_del_view(x, view);
		view = next;
	}
	lock_unlock(x->lock);
}

static int x_init(x_t* x) {
	memset(x, 0, sizeof(x_t));
	x->xwm_pid = -1;

	int fd = open("/dev/keyb0", O_RDONLY);
	x->keyb_fd = fd;

	fd = open("/dev/mouse0", O_RDONLY);
	x->mouse_fd = fd;

	fd = open("/dev/joystick", O_RDONLY);
	x->joystick_fd = fd;

	fd = open("/dev/fb1", O_RDWR);
	if(fd < 0) 
		fd = open("/dev/fb0", O_RDWR);
	if(fd < 0) {
		close(x->keyb_fd);
		close(x->mouse_fd);
		return -1;
	}
	x->fb_fd = fd;

	int id = dma(fd, NULL);
	if(id <= 0) {
		close(x->keyb_fd);
		close(x->mouse_fd);
		close(x->fb_fd);
		return -1;
	}

	void* gbuf = shm_map(id);
	if(gbuf == NULL) {
		close(x->keyb_fd);
		close(x->mouse_fd);
		close(x->fb_fd);
		return -1;
	}

	proto_t out;
	PF->init(&out, NULL, 0);

	if(fcntl_raw(fd, CNTL_INFO, NULL, &out) != 0) {
		shm_unmap(id);
		close(x->keyb_fd);
		close(x->mouse_fd);
		close(x->fb_fd);
		return -1;
	}

	int w = proto_read_int(&out);
	int h = proto_read_int(&out);
	x->g = graph_new(gbuf, w, h);
	PF->clear(&out);
	x->shm_id = id;
	x_dirty(x);

	x->cursor.size.w = 15;
	x->cursor.size.h = 15;
	x->cursor.offset.x = 8;
	x->cursor.offset.y = 8;
	x->cursor.cpos.x = w/2;
	x->cursor.cpos.y = h/2; 
	x->show_cursor = true;

	x->lock = lock_new();
	return 0;
}	

static int get_win_frame_pos(x_t* x, xview_t* view) {
	if((view->xinfo.style & X_STYLE_NO_FRAME) != 0)
		return -1;

	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->
		addi(&in, x->cursor.cpos.x)->
		addi(&in, x->cursor.cpos.y)->
		add(&in, &view->xinfo, sizeof(xinfo_t));
	ipc_call(x->xwm_pid, XWM_CNTL_GET_POS, &in, &out);
	PF->clear(&in);

	int res = proto_read_int(&out);
	PF->clear(&out);
	return res;
}

static xview_t* get_mouse_owner(x_t* x, int* win_frame_pos) {
	xview_t* view = x->view_tail;
	if(win_frame_pos != NULL)
		*win_frame_pos = -1;

	while(view != NULL) {
		if(!view->xinfo.visible) {
			view = view->prev;
			continue;
		}
		if(x->cursor.cpos.x >= view->xinfo.wsr.x &&
				x->cursor.cpos.x < (view->xinfo.wsr.x+view->xinfo.wsr.w) &&
				x->cursor.cpos.y >= view->xinfo.wsr.y &&
				x->cursor.cpos.y < (view->xinfo.wsr.y+view->xinfo.wsr.h))
			return view;
		int pos = get_win_frame_pos(x, view);
		if(pos >= 0) {
			if(win_frame_pos != NULL)
				*win_frame_pos = pos;
			return view;
		}
		view = view->prev;
	}
	return NULL;
}

static void mouse_cxy(x_t* x, int32_t rx, int32_t ry) {
	x->cursor.cpos.x += rx;
	x->cursor.cpos.y += ry;
	if(x->cursor.cpos.x < 0)
		x->cursor.cpos.x = 0;
	if(x->cursor.cpos.x >= (int32_t)x->g->w)
		x->cursor.cpos.x = x->g->w;
	if(x->cursor.cpos.y < 0)
		x->cursor.cpos.y = 0;
	if(x->cursor.cpos.y >= (int32_t)x->g->h)
		x->cursor.cpos.y = x->g->h;
}

static xview_t* get_top_view(x_t* x) {
	xview_t* ret = x->view_tail; 
	while(ret != NULL) {
		if(ret->xinfo.visible)
			return ret;
		ret = ret->prev;
	}
	return NULL;
}

static int mouse_handle(x_t* x, int8_t state, int32_t rx, int32_t ry) {
	mouse_cxy(x, rx, ry);

	xevent_t e;
	e.type = XEVT_MOUSE;
	e.state = XEVT_MOUSE_MOVE;
	e.value.mouse.x = x->cursor.cpos.x;
	e.value.mouse.y = x->cursor.cpos.y;
	e.value.mouse.rx = rx;
	e.value.mouse.ry = ry;

	int pos = -1;
	xview_t* view = NULL;
	if(x->current.view != NULL)
		view = x->current.view;
	else
		view = get_mouse_owner(x, &pos);

	if(view == NULL) {
		return -1;
	}

	if(state == 2) { //down
		if(view != x->view_tail) {
			remove_view(x, view);
			push_view(x, view);
		}

		if(pos == XWM_FRAME_CLOSE) { //window close
			e.type = XEVT_WIN;
			e.value.window.event = XEVT_WIN_CLOSE;
			view->xinfo.visible = false;
			x_dirty(x);
		}
		else if(pos == XWM_FRAME_MAX) {
			e.type = XEVT_WIN;
			e.value.window.event = XEVT_WIN_MAX;
		}
		else { // mouse down
			if(pos == XWM_FRAME_TITLE) {//window title 
				x->current.view = view;
				x->current.old_pos.x = x->cursor.cpos.x;
				x->current.old_pos.y = x->cursor.cpos.y;
				x_dirty(x);
			}
			e.state = XEVT_MOUSE_DOWN;
		}
	}
	else if(state == 1) {
		e.state = XEVT_MOUSE_UP;
		if(x->current.view == view) {
			x_dirty(x);
		}
		x->current.view = NULL;
	}

	if(x->current.view == view) {
		int mrx = x->cursor.cpos.x - x->current.old_pos.x;
		int mry = x->cursor.cpos.y - x->current.old_pos.y;
		if(abs32(mrx) > 16 || abs32(mry) > 16) {
			x->current.old_pos.x = x->cursor.cpos.x;
			x->current.old_pos.y = x->cursor.cpos.y;
			view->xinfo.wsr.x += mrx;
			view->xinfo.wsr.y += mry;
			x_dirty(x);
		}
	}
	x_push_event(view, &e);
	return -1;
}

static int keyb_handle(x_t* x, int8_t v) {
	xview_t* topv = get_top_view(x);
	if(topv != NULL) {
		xevent_t e;
		e.type = XEVT_KEYB;
		e.value.keyboard.value = v;

		//lock_lock(x->lock);
		x_push_event(topv, &e);
		//lock_unlock(x->lock);
	}
	usleep(1000);
	return 0;
}

#define KEY_V_UP        0x1
#define KEY_V_DOWN      0x2
#define KEY_V_LEFT      0x4
#define KEY_V_RIGHT     0x8
#define KEY_V_PRESS     0x10
#define KEY_V_1         0x20
#define KEY_V_2         0x40
#define KEY_V_3         0x80

static bool prs_down = false;
static bool j_mouse = true;

static void joy_2_mouse(int key, int8_t* mv) {
	mv[0] = mv[1] = mv[2] = 0;
	switch(key) {
	case KEY_V_UP:
		mv[2] -= 8;
		return;
	case KEY_V_DOWN:
		mv[2] += 8;
		return;
	case KEY_V_LEFT:
		mv[1] -= 8;
		return;
	case KEY_V_RIGHT:
		mv[1] += 8;
		return;
	case KEY_V_PRESS:
		if(!prs_down) {
			mv[0] = 2;
			prs_down = true;
		}
		return;
	}	
}

static void joy_2_keyb(int key, int8_t* v) {
	*v = 0;
	switch(key) {
	case KEY_V_UP:
		*v = X_KEY_UP;
		return;
	case KEY_V_DOWN:
		*v = X_KEY_DOWN;
		return;
	case KEY_V_LEFT:
		*v = X_KEY_LEFT;
		return;
	case KEY_V_RIGHT:
		*v = X_KEY_RIGHT;
		return;
	case KEY_V_1:
	case KEY_V_PRESS:
		*v = X_KEY_ENTER;
		return;
	case KEY_V_2:
		*v = X_KEY_ESC;
		return;
	}	
}

static void read_input(x_t* x) {
	//read keyb
	if(x->keyb_fd >= 0) {
		int8_t v;
		int rd = read_nblock(x->keyb_fd, &v, 1);
		if(rd == 1) {
			keyb_handle(x, v);
		}
	}

	//read mouse
	if(x->mouse_fd >= 0) {
		int8_t mv[4];
		if(read_nblock(x->mouse_fd, mv, 4) == 4) {
			//lock_lock(x->lock);
			mouse_handle(x, mv[0], mv[1], mv[2]);
			x->need_repaint = true;
			//lock_unlock(x->lock);
		}
	}

	//read joystick
	if(x->joystick_fd >= 0) {
		uint8_t key;
		int8_t mv[4];
		if(read(x->joystick_fd, &key, 1) == 1) {
			if(key == KEY_V_3 && !prs_down) { //switch joy mouse/keyboard mode
				j_mouse = !j_mouse;
				prs_down = true;
				x->show_cursor = j_mouse;
				x->need_repaint = true;
				if(x->show_cursor) {
					x->cursor.drop = true;
				}
			}

			if(j_mouse) {
				joy_2_mouse(key, mv);
				if(key == 0 && prs_down) {
					key = 1;
					prs_down = false;
					mv[0] = 1;
				}
				if(key != 0) {
					//lock_lock(x->lock);
					mouse_handle(x, mv[0], mv[1], mv[2]);
					x->need_repaint = true;
					//lock_unlock(x->lock);
				}
			}
			else {
				int8_t v;
				if(key == 0 && prs_down)
					prs_down = false;
				else {
					joy_2_keyb(key, &v);
					if(v != 0)
						keyb_handle(x, v);
				}
			}
		}
	}
}

static int xserver_loop_step(void* p) {
	x_t* x = (x_t*)p;
	x_check_views(x);

	/*const char* cc = get_global_str("system.current_console");
	if(cc[0] == 'x') {
		if(x->actived == 0) {
			x_dirty(x);
		}
		x->actived = 1;
	}
	else {
		x->actived = 0;
		usleep(10000);
		return -1;
	}
	*/
	x->actived = 1;

	read_input(x);

	lock_lock(x->lock);
	x_repaint(x);	
	lock_unlock(x->lock);
	return 0;
}

static void x_close(x_t* x) {
	lock_free(x->lock);
	close(x->keyb_fd);
	close(x->mouse_fd);
	graph_free(x->g);
	shm_unmap(x->shm_id);
	close(x->fb_fd);
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/x";

	x_t x;
	if(x_init(&x) != 0) {
		return -1;
	}
	read_config(&x, "/etc/x/x.conf");

	int xwm_pid = -1;
	if(x.config.xwm[0] != 0) {
		xwm_pid = fork();
		if(xwm_pid == 0) {
			exec(x.config.xwm);
		}
		proc_wait_ready(xwm_pid);
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xserver");
	dev.fcntl = xserver_fcntl;
	dev.close = xserver_close;
	dev.open = xserver_open;
	dev.info = xserver_info;
	dev.loop_step= xserver_loop_step;

	x.xwm_pid = xwm_pid;
	prs_down = false;
	j_mouse = true;
	//pthread_create(NULL, NULL, read_thread, &x);
	dev.extra_data = &x;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	x_close(&x);
	return 0;
}
