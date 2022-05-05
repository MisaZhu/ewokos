#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/basic_math.h>
#include <sys/shm.h>
#include <fb/fb.h>
#include <sys/ipc.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwm.h>
#include <sys/proc.h>
#include <sconf.h>
#include <screen/screen.h>
#include "cursor.h"

#define X_EVENT_MAX 16

enum {
	X_VIEW_DRAG_MOVE = 0,
	X_VIEW_DRAG_RESIZE
};

typedef struct st_xview {
	int fd;
	int from_pid;
	graph_t* g;
	xinfo_t xinfo;
	bool dirty;

	grect_t r_title;
	grect_t r_close;
	grect_t r_min;
	grect_t r_max;
	grect_t r_resize;

	struct st_xview *next;
	struct st_xview *prev;
} xview_t;

typedef struct {
	xview_t* view; //moving or resizing;
	gpos_t old_pos;
	gpos_t pos_delta;
	uint32_t drag_state;
} x_current_t;

typedef struct {
	uint32_t win_move_alpha;
	uint32_t fps;
	char xwm[128];
} x_conf_t;

typedef struct {
	bool actived;
	const char* scr_dev;
	fb_t fb;
	graph_t* g;
	int xwm_pid;
	bool dirty;
	bool need_repaint;
	bool show_cursor;
	cursor_t cursor;

	xview_t* view_head;
	xview_t* view_tail;
	xview_t* view_focus;
	xview_t* view_xim;

	x_current_t current;
	x_conf_t config;
} x_t;

static int32_t read_config(x_t* x, const char* fname) {
	x->config.win_move_alpha = 0x88;
	x->config.fps = 60;
	x->config.xwm[0] = 0;

	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL)
		return -1;

	const char* v = sconf_get(conf, "win_move_alpha");
	if(v[0] != 0) 
		x->config.win_move_alpha = atoi_base(v, 16);

	v = sconf_get(conf, "fps");
	if(v[0] != 0) 
		x->config.fps = atoi(v);

	v = sconf_get(conf, "xwm");
	if(v[0] != 0) 
		strncpy(x->config.xwm, v, 127);
	
	v = sconf_get(conf, "cursor");
	if(strcmp(v, "touch") == 0)
		x->cursor.type = CURSOR_TOUCH;
	else
		x->cursor.type = CURSOR_MOUSE;

	sconf_free(conf);
	return 0;
}

static void draw_win_frame(x_t* x, xview_t* view) {
	if((view->xinfo.style & X_STYLE_NO_FRAME) != 0 || x->g == NULL)
		return;

	proto_t in;

	PF->init(&in)->
		addi(&in, x->fb.dma_id)->
		addi(&in, x->g->w)->
		addi(&in, x->g->h)->
		add(&in, &view->xinfo, sizeof(xinfo_t));
	if(view == x->view_focus)
		PF->addi(&in, 1); //top win
	else
		PF->addi(&in, 0);

	ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_FRAME, &in, NULL);
	PF->clear(&in);
}

static void draw_desktop(x_t* x) {
	if(x->g == NULL)
		return;

	proto_t in;
	PF->init(&in)->
		addi(&in, x->fb.dma_id)->
		addi(&in, x->g->w)->
		addi(&in, x->g->h);

	int res = ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_DESKTOP, &in, NULL);
	PF->clear(&in);
	if(res != 0)
	 graph_fill(x->g, 0, 0, x->g->w, x->g->h, 0xff000000);
}

static void draw_drag_frame(x_t* xp) {
	if(xp->g == NULL)
		return;

	int x = xp->current.view->xinfo.wsr.x;
	int y = xp->current.view->xinfo.wsr.y;
	int w = xp->current.view->xinfo.wsr.w;
	int h = xp->current.view->xinfo.wsr.h;

	if(xp->current.drag_state == X_VIEW_DRAG_MOVE)  {
		x += xp->current.pos_delta.x;
		y += xp->current.pos_delta.y;
	}
	else if(xp->current.drag_state == X_VIEW_DRAG_RESIZE)  {
		w += xp->current.pos_delta.x;
		h += xp->current.pos_delta.y;
	}

	grect_t r = {x, y, w, h};

	proto_t in;
	PF->init(&in)->
		addi(&in, xp->fb.dma_id)->
		addi(&in, xp->g->w)->
		addi(&in, xp->g->h)->
		add(&in, &r, sizeof(grect_t));

	ipc_call_wait(xp->xwm_pid, XWM_CNTL_DRAW_DRAG_FRAME, &in, NULL);
	PF->clear(&in);
}

static int draw_view(x_t* xp, xview_t* view) {
	if(!xp->dirty && !view->dirty)
		return 0;

	if(view->g != NULL) {
		if((view->xinfo.style & X_STYLE_ALPHA) != 0) {
			graph_blt_alpha(view->g, 0, 0, 
					view->xinfo.wsr.w,
					view->xinfo.wsr.h,
					xp->g,
					view->xinfo.wsr.x,
					view->xinfo.wsr.y,
					view->xinfo.wsr.w,
					view->xinfo.wsr.h, 0xff);
		}
		else {
			graph_blt(view->g, 0, 0, 
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
	if(xp->current.view == view && xp->config.win_move_alpha < 0xff) //drag and moving
		draw_drag_frame(xp);
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

static inline void send_event(int32_t pid, xevent_t* e) {
	proto_t in;
	PF->init(&in)->add(&in, e, sizeof(xevent_t));
	ipc_call(pid, X_CMD_PUSH_EVENT, &in, NULL);
	PF->clear(&in);
}

static void x_push_event(x_t* x, xview_t* view, xevent_t* e) {
	(void)x;
	if(view->from_pid <= 0)
		return;
	e->win = view->xinfo.win;
	send_event(view->from_pid, e);
}

static void hide_view(x_t* x, xview_t* view) {
	if(view == NULL)
		return;

	xevent_t e;
	e.type = XEVT_WIN;
	e.value.window.event = XEVT_WIN_VISIBLE;
	e.value.window.v0 = 0;
	x_push_event(x, view, &e);
}

static void show_view(x_t* x, xview_t* view) {
	if(view == NULL)
		return;

	xevent_t e;
	e.type = XEVT_WIN;
	e.value.window.event = XEVT_WIN_VISIBLE;
	e.value.window.v0 = 1;
	x_push_event(x, view, &e);
}

static void try_focus(x_t* x, xview_t* view) {
	if(x->view_focus == view)
		return;
	if((view->xinfo.style & X_STYLE_NO_FOCUS) == 0) {
		hide_view(x, x->view_xim);
		if(x->view_focus != NULL) {
			xevent_t e;
			e.type = XEVT_WIN;
			e.value.window.event = XEVT_WIN_UNFOCUS;
			x_push_event(x, x->view_focus, &e);
		}

		xevent_t e;
		e.type = XEVT_WIN;
		e.value.window.event = XEVT_WIN_FOCUS;
		x_push_event(x, view, &e);
		x->view_focus = view;
	}
}

static void push_view(x_t* x, xview_t* view) {
	if((view->xinfo.style & X_STYLE_SYSBOTTOM) != 0) { //push head if sysbottom style
		if(x->view_head != NULL) {
			x->view_head->prev = view;
			view->next = x->view_head;
			x->view_head = view;
		}
		else {
			x->view_tail = x->view_head = view;
		}
	}
	else if((view->xinfo.style & X_STYLE_SYSTOP) != 0) { //push tail if systop style
		if(x->view_tail != NULL) {
			x->view_tail->next = view;
			view->prev = x->view_tail;
			x->view_tail = view;
		}
		else {
			x->view_tail = x->view_head = view;
		}
	}
	else { 
		xview_t* view_top = x->view_tail;
		xview_t* view_systop = NULL;
		while(view_top != NULL) {
			if((view_top->xinfo.style & X_STYLE_SYSTOP) == 0)
				break;
			view_systop = view_top;
			view_top = view_top->prev;
		}

		if(view_top != NULL) {
			view->next = view_top->next;
			if(view_top->next != NULL)
				view_top->next->prev = view;
			else 
				x->view_tail = view;

			view_top->next = view;
			view->prev = view_top;
		}
		else {
			x->view_head = view;
			if(view_systop != NULL)  {
				if(view_systop->prev != NULL)
					view_systop->prev->next = view;

				view->prev = view_systop->prev;
				view_systop->prev = view;
				view->next = view_systop;
			}
			else {
				x->view_tail = view;
			}
		}
	}

	try_focus(x, view);
	if(view->xinfo.visible)
		x_dirty(x);
}

static xview_t* get_next_focus_view(x_t* x) {
	xview_t* ret = x->view_tail; 
	while(ret != NULL) {
		if(ret->xinfo.visible && (ret->xinfo.style & X_STYLE_NO_FOCUS) == 0)
			return ret;
		ret = ret->prev;
	}
	return NULL;
}

static void x_del_view(x_t* x, xview_t* view) {
	if(view == x->view_focus)
		hide_view(x, x->view_xim);

	remove_view(x, view);
	free(view);
	x->view_focus = get_next_focus_view(x);
	if(x->view_focus != NULL) {
		xevent_t e;
		e.type = XEVT_WIN;
		e.value.window.event = XEVT_WIN_FOCUS;
		x_push_event(x, x->view_focus, &e);
	}
}

static void hide_cursor(x_t* x) {
	if(x->cursor.drop || x->g == NULL)
		return;

	if(x->cursor.g == NULL) {
		x->cursor.g = graph_new(NULL, x->cursor.size.w, x->cursor.size.h);
		graph_blt(x->g,
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
		graph_blt(x->cursor.g,
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

static inline void refresh_cursor(x_t* x) {
	if(x->g == NULL || x->cursor.g == NULL)
		return;
	int32_t mx = x->cursor.cpos.x - x->cursor.offset.x;
	int32_t my = x->cursor.cpos.y - x->cursor.offset.y;
	int32_t mw = x->cursor.size.w;
	int32_t mh = x->cursor.size.h;

	graph_blt(x->g, mx, my, mw, mh,
			x->cursor.g, 0, 0, mw, mh);

	draw_cursor(x->g, &x->cursor, mx, my);

	x->cursor.old_pos.x = x->cursor.cpos.x;
	x->cursor.old_pos.y = x->cursor.cpos.y;
	x->cursor.drop = false;
}

static void x_reset(x_t* x) {
	x->g = fb_fetch_graph(&x->fb);
}

static int x_init(const char* scr_dev, x_t* x) {
	memset(x, 0, sizeof(x_t));
	x->scr_dev = scr_dev;
	const char* fb_dev = get_scr_fb_dev(scr_dev);
	x->xwm_pid = -1;

	if(fb_open(fb_dev, &x->fb) != 0)
		return -1;

	x_reset(x);
	x_dirty(x);

	x->cursor.cpos.x = x->g->w/2;
	x->cursor.cpos.y = x->g->h/2; 
	x->show_cursor = true;

	x->dirty = true;
	x->actived = true;
	return 0;
}	


static void x_close(x_t* x) {
	fb_close(&x->fb);
}

static void x_repaint(x_t* x) {
	if(x->g == NULL ||
			!x->actived ||
			(!x->need_repaint) ||
			!is_scr_top(x->scr_dev))
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
		refresh_cursor(x);

	fb_flush(&x->fb);

	if(undirty) {
		x->dirty = false;
	}
}

static inline void x_repaint_req(x_t* x) {
	x->need_repaint = true;
}

static xview_t* x_get_view(x_t* x, int fd, int from_pid) {
	xview_t* view = x->view_head;
	while(view != NULL) {
		if(view->fd == fd && view->from_pid == from_pid)
			return view;
		view = view->next;
	}
	return NULL;
}

static xview_t* get_first_visible_view(x_t* x) {
	xview_t* ret = x->view_tail; 
	while(ret != NULL) {
		if(ret->xinfo.visible)
			return ret;
		ret = ret->prev;
	}
	return NULL;
}

static int x_update(int fd, int from_pid, x_t* x) {
	if(fd < 0)
		return -1;
	
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL)
		return -1;
	if(!view->xinfo.visible)
		return 0;

	view->dirty = true;
	if(view != get_first_visible_view(x) ||
			(view->xinfo.style & X_STYLE_ALPHA) != 0) {
		x_dirty(x);
	}
	x_repaint_req(x);
	return 0;
}

static int x_set_visible(int fd, int from_pid, proto_t* in, x_t* x) {
	if(fd < 0)
		return -1;
	
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL)
		return -1;

	view->xinfo.visible = proto_read_int(in);
	view->dirty = true;
	x_dirty(x);
	return 0;
}

static int x_update_frame_areas(x_t* x, xview_t* view) {
	if((view->xinfo.style & X_STYLE_NO_FRAME) != 0)
		return -1;

	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->
		add(&in, &view->xinfo, sizeof(xinfo_t));
	int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_FRAME_AREAS, &in, &out);
	PF->clear(&in);

	proto_read_to(&out, &view->r_title, sizeof(grect_t));
	proto_read_to(&out, &view->r_close, sizeof(grect_t));
	proto_read_to(&out, &view->r_min, sizeof(grect_t));
	proto_read_to(&out, &view->r_max, sizeof(grect_t));
	proto_read_to(&out, &view->r_resize, sizeof(grect_t));
	PF->clear(&out);
	return res;
}

static void x_get_min_size(x_t* x, xview_t* view, int *w, int* h) {
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->
		add(&in, &view->xinfo, sizeof(xinfo_t));
	int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_MIN_SIZE, &in, &out);
	PF->clear(&in);
	if(res == 0) { 
		*w = proto_read_int(&out);
		*h = proto_read_int(&out);
	}
	PF->clear(&out);
}

static int x_update_info_raw(int fd, int from_pid, proto_t* in, x_t* x) {
	xinfo_t xinfo;
	int sz = sizeof(xinfo_t);
	if(fd < 0 || proto_read_to(in, &xinfo, sz) != sz)
		return -1;
	
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL)
		return -1;

	if((xinfo.style & X_STYLE_XIM) != 0)
		x->view_xim = view;
	
	if((xinfo.style & X_STYLE_NO_FRAME) == 0 &&
      (xinfo.style & X_STYLE_NO_TITLE) == 0) {
		int minw = 0, minh = 0;
		x_get_min_size(x, view, &minw, &minh);
		if(xinfo.wsr.w < minw)
			xinfo.wsr.w = minw;
		if(xinfo.wsr.h < minh)
			xinfo.wsr.h = minh;
	}

	if(xinfo.wsr.w <= 0 || xinfo.wsr.h <= 0)
		return -1;
	
	int shm_id = view->xinfo.shm_id;
	if(shm_id == 0 ||
			view->g == NULL ||
			view->xinfo.wsr.w != xinfo.wsr.w ||
			view->xinfo.wsr.h != xinfo.wsr.h) {
		if(view->g != NULL && shm_id > 0) {
			graph_free(view->g);
			shm_unmap(shm_id);
			view->g = NULL;
		}
		shm_id = shm_alloc(xinfo.wsr.w * xinfo.wsr.h * 4, 1);
		void* p = shm_map(shm_id);
		if(p == NULL) 
			return -1;
		view->g = graph_new(p, xinfo.wsr.w, xinfo.wsr.h);
		graph_clear(view->g, 0x0);
	}
	view->dirty = true;
	x_repaint_req(x);

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
	x_update_frame_areas(x, view);

	return 0;
}

static int x_update_info(int fd, int from_pid, proto_t* in, x_t* x) {
	int res = x_update_info_raw(fd, from_pid, in, x);
	return res;
}

static int x_get_info(int fd, int from_pid, x_t* x, proto_t* out) {
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL)
		return -1;
	PF->add(out, &view->xinfo, sizeof(xinfo_t));
	return 0;
}

static int get_xwm_workspace(x_t* x, int style, grect_t* rin, grect_t* rout) {
	proto_t in, out;
	PF->init(&out);

	PF->init(&in)->
		addi(&in, style)->
		add(&in, rin, sizeof(grect_t));

	int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_WORKSPACE, &in, &out);
	PF->clear(&in);
	if(res == 0)
		proto_read_to(&out, rout, sizeof(grect_t));
	PF->clear(&out);

	return res;
}

static int x_workspace(x_t* x, proto_t* in, proto_t* out) {
	grect_t r;
	int style = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));
	get_xwm_workspace(x, style, &r, &r); 
	PF->add(out, &r, sizeof(grect_t));
	return 0;
}

static int x_call_xim(x_t* x) {
	show_view(x, x->view_xim);
	return 0;
}

static int xserver_fcntl(int fd, int from_pid, fsinfo_t* info,
		int cmd, proto_t* in, proto_t* out, void* p) {
	(void)fd;
	(void)info;
	x_t* x = (x_t*)p;

	int res = -1;
	if(cmd == X_CNTL_UPDATE) {
		res = x_update(fd, from_pid, x);
	}	
	else if(cmd == X_CNTL_UPDATE_INFO) {
		res = x_update_info(fd, from_pid, in, x);
	}
	else if(cmd == X_CNTL_SET_VISIBLE) {
		res = x_set_visible(fd, from_pid, in, x);
	}
	else if(cmd == X_CNTL_GET_INFO) {
		res = x_get_info(fd, from_pid, x, out);
	}
	else if(cmd == X_CNTL_WORKSPACE) {
		res = x_workspace(x, in, out);
	}
	else if(cmd == X_CNTL_CALL_XIM) {
		res = x_call_xim(x);
	}
	return res;
}

static int xserver_open(int fd, int from_pid, fsinfo_t* info, int oflag, void* p) {
	(void)oflag;
	(void)info;
	if(fd < 0)
		return -1;

	x_t* x = (x_t*)p;
	xview_t* view = (xview_t*)malloc(sizeof(xview_t));
	if(view == NULL)
		return -1;

	memset(view, 0, sizeof(xview_t));
	view->fd = fd;
	view->from_pid = from_pid;
	push_view(x, view);
	return 0;
}

static void mouse_cxy(x_t* x, int32_t rx, int32_t ry) {
	x->cursor.cpos.x += rx;
	x->cursor.cpos.y += ry;

	if(x->cursor.cpos.x < 0)
		//x->cursor.cpos.x += x->g->w;
		x->cursor.cpos.x = 0;

	if(x->cursor.cpos.x > (int32_t)x->g->w)
		//x->cursor.cpos.x = x->g->w - x->cursor.cpos.x;
		x->cursor.cpos.x = x->g->w;

	if(x->cursor.cpos.y < 0)
		//x->cursor.cpos.y += x->g->h;
		x->cursor.cpos.y = 0;

	if(x->cursor.cpos.y >= (int32_t)x->g->h)
		//x->cursor.cpos.y = x->g->h - x->cursor.cpos.y;
		x->cursor.cpos.y = x->g->h;
}

enum {
	FRAME_R_TITLE = 0,
	FRAME_R_CLOSE,
	FRAME_R_MIN,
	FRAME_R_MAX,
	FRAME_R_RESIZE
};

static int get_win_frame_pos(x_t* x, xview_t* view) {
	if((view->xinfo.style & X_STYLE_NO_FRAME) != 0)
		return -1;

	int res = -1;
	if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &view->r_title))
		res = FRAME_R_TITLE;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &view->r_close))
		res = FRAME_R_CLOSE;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &view->r_min))
		res = FRAME_R_MIN;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &view->r_max))
		res = FRAME_R_MAX;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &view->r_resize))
		res = FRAME_R_RESIZE;
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
		int pos = get_win_frame_pos(x, view);
		if(pos >= 0) {
			if(win_frame_pos != NULL)
				*win_frame_pos = pos;
			return view;
		}
		if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &view->xinfo.wsr))
			return view;
		view = view->prev;
	}
	return NULL;
}

static int mouse_handle(x_t* x, xevent_t* ev) {
	if(ev->value.mouse.relative != 0) {
		mouse_cxy(x, ev->value.mouse.rx, ev->value.mouse.ry);
		ev->value.mouse.x = x->cursor.cpos.x;
		ev->value.mouse.y = x->cursor.cpos.y;
	}
	else {
		x->cursor.cpos.x = ev->value.mouse.x;
		x->cursor.cpos.y = ev->value.mouse.y;
	}

	if(ev->state ==  XEVT_MOUSE_DOWN)
		x->cursor.down = true;
	else if(ev->state ==  XEVT_MOUSE_UP)
		x->cursor.down = false;
	

	int pos = -1;
	xview_t* view = NULL;
	if(x->current.view != NULL)
		view = x->current.view;
	else
		view = get_mouse_owner(x, &pos);

	if(view == NULL)
		return -1;
	ev->value.mouse.winx = ev->value.mouse.x - view->xinfo.wsr.x;
	ev->value.mouse.winy = ev->value.mouse.y - view->xinfo.wsr.y;

	if(ev->state ==  XEVT_MOUSE_DOWN) {
		if(view != x->view_tail) {
			remove_view(x, view);
			push_view(x, view);
		}
		else {
			try_focus(x, view);
		}

		if(pos == FRAME_R_TITLE) {//window title 
			x->current.view = view;
			x->current.old_pos.x = x->cursor.cpos.x;
			x->current.old_pos.y = x->cursor.cpos.y;
			x->current.drag_state = X_VIEW_DRAG_MOVE;
			x_dirty(x);
		}
		else if(pos == FRAME_R_RESIZE) {//window resize
			x->current.view = view;
			x->current.old_pos.x = x->cursor.cpos.x;
			x->current.old_pos.y = x->cursor.cpos.y;
			x->current.drag_state = X_VIEW_DRAG_RESIZE;
			x_dirty(x);
		}
		ev->state = XEVT_MOUSE_DOWN;
	}
	else if(ev->state == XEVT_MOUSE_UP) {
		if(x->current.view == view) {
			ev->type = XEVT_WIN;
			ev->value.window.v0 =  x->current.pos_delta.x;
			ev->value.window.v1 =  x->current.pos_delta.y;
			if(x->current.drag_state == X_VIEW_DRAG_RESIZE) {
				ev->value.window.event = XEVT_WIN_RESIZE;
			}
			else if(x->current.drag_state == X_VIEW_DRAG_MOVE) {
				ev->value.window.event = XEVT_WIN_MOVE;
			}
			x->current.pos_delta.x = 0;
			x->current.pos_delta.y = 0;
		}
		else if(pos == FRAME_R_CLOSE) { //window close
			ev->type = XEVT_WIN;
			ev->value.window.event = XEVT_WIN_CLOSE;
			//view->xinfo.visible = false;
			//x_dirty(x);
		}
		else if(pos == FRAME_R_MAX) {
			ev->type = XEVT_WIN;
			ev->value.window.event = XEVT_WIN_MAX;
		}
		x->current.view = NULL;
	}

	if(x->current.view == view) {
		int mrx = x->cursor.cpos.x - x->current.old_pos.x;
		int mry = x->cursor.cpos.y - x->current.old_pos.y;
		if(abs32(mrx) > 15 || abs32(mry) > 15) {
			x->current.pos_delta.x = mrx;
			x->current.pos_delta.y = mry;
			x_dirty(x);
		}
	}
	x_push_event(x, view, ev);
	return -1;
}

static int im_handle(x_t* x, xevent_t* ev) {
	if(x->view_focus != NULL) {
		x_push_event(x, x->view_focus, ev);
	}
	return 0;
}

static void handle_input(x_t* x, xevent_t* ev) {
	if(ev->type == XEVT_IM) {
		im_handle(x, ev);
	}
	else if(ev->type == XEVT_MOUSE) {
		mouse_handle(x, ev);
	}
	x_repaint_req(x);
}

static int xserver_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	(void)in;
	x_t* x = (x_t*)p;

	if(cmd == DEV_CNTL_REFRESH) {
		x_dirty(x);
	}
	else if(cmd == X_DCNTL_GET_INFO) {
		xscreen_t scr;	
		scr.id = 0;
		scr.fps = x->config.fps;
		scr.size.w = x->g->w;
		scr.size.h = x->g->h;
		PF->add(ret, &scr, sizeof(xscreen_t));
	}
	else if(cmd == X_DCNTL_SET_XWM) {
		x->xwm_pid = from_pid;
		x_dirty(x);
	}
	else if(cmd == X_DCNTL_UNSET_XWM) {
		x->xwm_pid = -1;
		x_dirty(x);
	}
	else if(cmd == X_DCNTL_INPUT) {
		xevent_t ev;
		proto_read_to(in, &ev, sizeof(xevent_t));
		handle_input(x, &ev);
	}
	return 0;
}

static int xserver_close(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)info;
	(void)fd;
	x_t* x = (x_t*)p;
	xview_t* view = x_get_view(x, fd, from_pid);
	if(view == NULL) {
		return -1;
	}
	x_del_view(x, view);	
	return 0;
}

int xserver_step(void* p) {
	x_t* x = (x_t*)p;
	ipc_disable();
	x_repaint(x);
	ipc_enable();
	usleep(1000000/x->config.fps);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/x";
	const char* scr_dev = argc > 2 ? argv[2]: "/dev/scr0";

	x_t x;
	if(x_init(scr_dev, &x) != 0)
		return -1;
	read_config(&x, "/etc/x/x.conf");
	cursor_init(&x.cursor);

	int pid = -1;
	if(x.config.xwm[0] != 0) {
		pid = fork();
		if(pid == 0) {
			exec(x.config.xwm);
		}
		proc_wait_ready(pid);
		x.xwm_pid = pid;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xserver");
	dev.fcntl = xserver_fcntl;
	dev.close = xserver_close;
	dev.open = xserver_open;
	dev.dev_cntl = xserver_dev_cntl;
	dev.loop_step = xserver_step;
	dev.extra_data = &x;

	dev_cntl(x.scr_dev, SCR_SET_TOP, NULL, NULL);

	x_repaint_req(&x);
	x_repaint(&x);
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	x_close(&x);
	return 0;
}
