#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/core.h>
#include <ewoksys/syscall.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <sys/shm.h>
#include <fb/fb.h>
#include <ewoksys/ipc.h>
#include <x/xcntl.h>
#include <x/xtheme.h>
#include <x/xevent.h>
#include <x/xwm.h>
#include <ewoksys/proc.h>
#include <graph/graph_png.h>
#include <ewoksys/keydef.h>
#include <tinyjson/tinyjson.h>
#include <display/display.h>
#include "xserver.h"
#include "xtheme.h"

static int32_t read_config(x_t* x, const char* fname) {
	x->config.fps = 60;
	x->config.bg_run = 0;

	json_var_t *conf_var = json_parse_file(fname);	

	x->config.fps = json_get_int_def(conf_var, "fps", 30);
	x->config.bg_run = json_get_int_def(conf_var, "bg_run", 0);
	x->config.gray_mode = json_get_int_def(conf_var, "gray_mode", 0);
	x->config.bg_proc_priority = json_get_int_def(conf_var, "bg_proc_priority", 2);

	const char* v = json_get_str_def(conf_var, "logo", "/usr/system/icons/xlogo.png");
	x->config.logo = png_image_new(v);

	v = json_get_str_def(conf_var, "cursor", "");
	if(strcmp(v, "touch") == 0)
		x->cursor.type = CURSOR_TOUCH;
	else if(strcmp(v, "mouse") == 0)
		x->cursor.type = CURSOR_MOUSE;
	else {
		if(strcmp(v, "none") == 0)
			x->show_cursor = false;
	}
	if(conf_var != NULL)
		json_var_unref(conf_var);
	return 0;
}

static bool check_xwm(x_t* x) {
	if(x->xwm_pid < 0) {
		x->xwm_uuid = 0;
		return false;
	}

	if(proc_check_uuid(x->xwm_pid, x->xwm_uuid) == x->xwm_uuid)
		return true;

	x->xwm_pid = -1;
	x->xwm_uuid = 0;
	return false;
}

static bool top_proc(x_t* x, xwin_t* win) {
	if(x->win_focus == NULL)
		return false;
	if(win->from_main_pid == x->win_focus->from_main_pid)
		return true;
	return false;
}

static void prepare_win_content(x_t* x, xwin_t* win) {
	if(!check_xwm(x))
		return;

	x_display_t *display = &x->displays[win->xinfo->display_index];
	if(display->g == NULL)
		return;

	if(win->dirty || win->xinfo->focused || (win->xinfo->style & XWIN_STYLE_NO_BG_EFFECT) != 0)
		win->frame_dirty = true;
	
	if(!win->frame_dirty)
		return;

	graph_clear(win->frame_g, 0);
	graph_blt(win->ws_g, 0, 0, win->ws_g->w, win->ws_g->h,
			win->frame_g,
			win->xinfo->wsr.x - win->xinfo->winr.x,
			win->xinfo->wsr.y - win->xinfo->winr.y,
			win->xinfo->wsr.w,
			win->xinfo->wsr.h);
	
	proto_t in;
	PF->format(&in, "i,i,i,m",
		display->g_shm_id,
		display->g->w,
		display->g->h,
		win->xinfo, sizeof(xinfo_t));

	if(top_proc(x, win))
		PF->addi(&in, 1); //top win
	else
		PF->addi(&in, 0);

	ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_FRAME, &in);
	//ipc_call(x->xwm_pid, XWM_CNTL_DRAW_FRAME, &in, NULL);
	PF->clear(&in);
}

static void draw_init_desktop(x_t* x, x_display_t *display) {
	graph_draw_dot_pattern(display->g, 0, 0, display->g->w, display->g->h,
			0xff888888, 0xffffffff, 2, 1);
	
	if(x->config.logo != NULL) {
		graph_blt_alpha(x->config.logo, 0, 0, x->config.logo->w, x->config.logo->h,
			display->g, 
			(display->g->w - x->config.logo->w)/2,
			(display->g->h - x->config.logo->h)/2,
			x->config.logo->w,
			x->config.logo->h,
			0xff);
	}
}

static void draw_desktop(x_t* x, uint32_t display_index) {
	x_display_t *display = &x->displays[display_index];
	if(display->g == NULL)
		return;

	if(!check_xwm(x)) {
		draw_init_desktop(x, display);
		return;
	}	
	else if(x->config.logo != NULL) {
		graph_free(x->config.logo);
		x->config.logo = NULL;
	}

	proto_t in;
	PF->format(&in, "i,i,i",
		display->g_shm_id,
		display->g->w,
		display->g->h);

	int res = ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_DESKTOP, &in);
	PF->clear(&in);
	if(res != 0)
		graph_fill(display->g, 0, 0, display->g->w, display->g->h, 0xff000000);
}

static void draw_drag_frame(x_t* xp, uint32_t display_index) {
	x_display_t *display = &xp->displays[display_index];
	if(display->g == NULL)
		return;

	int x = xp->current.win_drag->xinfo->winr.x;
	int y = xp->current.win_drag->xinfo->winr.y;
	int w = xp->current.win_drag->xinfo->winr.w - xp->config.xwm_theme.shadow;
	int h = xp->current.win_drag->xinfo->winr.h - xp->config.xwm_theme.shadow;

	if(xp->current.drag_state == X_win_DRAG_MOVE)  {
		x += xp->current.pos_delta.x;
		y += xp->current.pos_delta.y;
	}
	else if(xp->current.drag_state == X_win_DRAG_RESIZE)  {
		w += xp->current.pos_delta.x;
		h += xp->current.pos_delta.y;
	}

	grect_t r = {x, y, w, h};

	proto_t in;
	PF->format(&in, "i,i,i,m",
		display->g_shm_id,
		display->g->w,
		display->g->h,
		&r, sizeof(grect_t));

	if(check_xwm(xp))
		ipc_call_wait(xp->xwm_pid, XWM_CNTL_DRAW_DRAG_FRAME, &in);
	PF->clear(&in);
}

static int draw_win(graph_t* disp_g, x_t* x, xwin_t* win) {
	prepare_win_content(x, win);

	graph_t* g = win->frame_g;
	if(g != NULL) {
		bool do_alpha = false;
		if(win->xinfo->alpha ||
					x->config.xwm_theme.alpha || 
					x->config.xwm_theme.shadow > 0) {
			do_alpha = true;
		}

		if(1) {
			graph_blt_alpha(g, 0, 0, 
					win->xinfo->winr.w,
					win->xinfo->winr.h,
					disp_g,
					win->xinfo->winr.x,
					win->xinfo->winr.y,
					win->xinfo->winr.w,
					win->xinfo->winr.h, 0xff);
		}
		else {
			graph_blt(g, 0, 0, 
					win->xinfo->winr.w,
					win->xinfo->winr.h,
					disp_g,
					win->xinfo->winr.x,
					win->xinfo->winr.y,
					win->xinfo->winr.w,
					win->xinfo->winr.h);
		}
	}

	win->dirty = false;
	win->frame_dirty = false;
	return 0;
}

static int drag_win(graph_t* disp_g, x_t* x, xwin_t* win) {
	if(x->current.win_drag == win &&
			(win->xinfo->style & XWIN_STYLE_NO_FRAME) == 0 &&
			win->xinfo->state != XWIN_STATE_MAX) {
		draw_drag_frame(x, win->xinfo->display_index);
		return 0;
	}
	return -1;
}

static inline void x_dirty(x_t* x, int32_t display_index) {
	if(display_index >= 0) {
		x_display_t *display = &x->displays[display_index];
		display->dirty = true;
		display->need_repaint = true;
		return;
	}

	for(uint32_t i=0; i<x->display_num; i++) {
		x_display_t *display = &x->displays[i];
		display->dirty = true;
		display->need_repaint = true;
	}
}

static void remove_win(x_t* x, xwin_t* win) {
	xwin_t* prev = win->prev;
	while(prev != NULL) {
		prev = prev->prev;
	}

	if(win->prev != NULL)
		win->prev->next = win->next;
	if(win->next != NULL)
		win->next->prev = win->prev;
	if(x->win_tail == win)
		x->win_tail = win->prev;
	if(x->win_head == win)
		x->win_head = win->next;
	win->next = win->prev = NULL;

	if(win->xinfo != NULL)
		x_dirty(x, win->xinfo->display_index);
	else
		x_dirty(x, -1);
}

static void x_quit(int from_pid) {
	xevent_remove(from_pid);
}

static void x_get_event(int from_pid, proto_t* out) {
	xevent_t evt;
	if(!xevent_pop(from_pid, &evt)) {
		PF->addi(out, -1);
		return;
	}
	PF->addi(out, 0)->add(out, &evt, sizeof(xevent_t));
}

static void x_push_event(x_t* x, xwin_t* win, xevent_t* e) {
	(void)x;
	if(win == NULL || win->from_pid <= 0 || win->xinfo == NULL)
		return;
	e->win = win->xinfo->win;
	xevent_push(win->from_pid, e);
	proc_wakeup(X_EVT_BLOCK_EVT);
}

static void hide_win(x_t* x, xwin_t* win) {
	x->im_state.win_xim_actived = false;
	if(win == NULL)
		return;

	xevent_t e;
	e.type = XEVT_WIN;
	e.value.window.event = XEVT_WIN_VISIBLE;
	e.value.window.v0 = 0;
	x_push_event(x, win, &e);
}

static void show_win(x_t* x, xwin_t* win) {
	if(win == NULL)
		return;

	x->im_state.win_xim_actived = true;
	xevent_t e;
	e.type = XEVT_WIN;
	e.value.window.event = XEVT_WIN_VISIBLE;
	e.value.window.v0 = 1;
	x_push_event(x, win, &e);
}

static void x_unfocus(x_t* x) {
	if(x->win_focus == NULL)
		return;

	hide_win(x, x->im_state.win_xim);
	xevent_t e;
	e.type = XEVT_WIN;
	e.value.window.event = XEVT_WIN_UNFOCUS;
	x->win_focus->xinfo->focused = false;
	x->win_focus->frame_dirty = true;
	x_push_event(x, x->win_focus, &e);

	proc_priority(x->win_focus->from_pid, x->config.bg_proc_priority);
	x->win_focus = NULL;
}

static void try_focus(x_t* x, xwin_t* win) {
	if(x->win_focus == win || win->xinfo == NULL) 
		return;
	if((win->xinfo->style & XWIN_STYLE_NO_FOCUS) == 0 && 
			(win->xinfo->style & XWIN_STYLE_LAZY) == 0) {
		x_unfocus(x);
		xevent_t e;
		e.type = XEVT_WIN;
		e.value.window.event = XEVT_WIN_FOCUS;
		win->xinfo->focused = true;
		win->frame_dirty = true;
		x_push_event(x, win, &e);
		x->win_focus = win;

		proc_priority(x->win_focus->from_pid, 0);
	}
}

static inline void x_repaint_req(x_t* x, int32_t display_index) {
	if(display_index >= 0) {
		x_display_t *display = &x->displays[display_index];
		display->need_repaint = true;
		return;
	}

	for(uint32_t i=0; i<x->display_num; i++) {
		x_display_t *display = &x->displays[i];
		display->need_repaint = true;
	}
}

static void push_win(x_t* x, xwin_t* win) {
	if((win->xinfo->style & XWIN_STYLE_SYSBOTTOM) != 0) { //push head if sysbottom style
		if(x->win_head != NULL) {
			x->win_head->prev = win;
			win->next = x->win_head;
			x->win_head = win;
		}
		else {
			x->win_tail = x->win_head = win;
		}
	}
	else if((win->xinfo->style & XWIN_STYLE_SYSTOP) != 0) { //push tail if systop style
		if(x->win_tail != NULL) {
			x->win_tail->next = win;
			win->prev = x->win_tail;
			x->win_tail = win;
		}
		else {
			x->win_tail = x->win_head = win;
		}
	}
	else { 
		xwin_t* win_top = x->win_tail;
		xwin_t* win_systop = NULL;
		while(win_top != NULL) {
			if((win_top->xinfo->style & XWIN_STYLE_SYSTOP) == 0)
				break;
			win_systop = win_top;
			win_top = win_top->prev;
		}

		if(win_top != NULL) {
			win->next = win_top->next;
			if(win_top->next != NULL)
				win_top->next->prev = win;
			else 
				x->win_tail = win;

			win_top->next = win;
			win->prev = win_top;
		}
		else {
			x->win_head = win;
			if(win_systop != NULL)  {
				if(win_systop->prev != NULL)
					win_systop->prev->next = win;

				win->prev = win_systop->prev;
				win_systop->prev = win;
				win->next = win_systop;
			}
			else {
				x->win_tail = win;
			}
		}
	}
	if(win->xinfo != NULL && win->xinfo->visible)
		try_focus(x, win);
}

static xwin_t* get_next_focus_win(x_t* x, bool skip_launcher) {
	xwin_t* ret = x->win_tail; 
	while(ret != NULL) {
		if(ret->xinfo->visible &&
				(ret->xinfo->style & XWIN_STYLE_NO_FOCUS) == 0 &&
				(!skip_launcher || ret != x->win_launcher))
			return ret;
		ret = ret->prev;
	}
	return NULL;
}

static void x_del_win(x_t* x, xwin_t* win) {
	if(win == x->win_focus)
		hide_win(x, x->im_state.win_xim);

	remove_win(x, win);
	if(win == x->win_last)
		x->win_last = NULL;


	if(win->ws_g != NULL) {
		graph_free(win->ws_g);
		shmdt(win->ws_g_shm);
	}

	if(win->frame_g != NULL) {
		graph_free(win->frame_g);
		shmdt(win->frame_g_shm);
	}

	shmdt(win->xinfo);
	if(win == x->win_focus)
		x->win_focus = NULL;
	free(win);
	win = get_next_focus_win(x, false);
	x->win_last = get_next_focus_win(x, true);

	if(win != NULL) {
		try_focus(x, win);
	}
}

static void hide_cursor(x_t* x) {
	x_display_t* display = &x->displays[x->current_display];
	if(x->cursor.drop || display->g == NULL)
		return;

	if(x->cursor.saved == NULL) {
		x->cursor.saved = graph_new(NULL, x->cursor.size.w, x->cursor.size.h);
		graph_blt(display->g,
				x->cursor.old_pos.x - x->cursor.offset.x,
				x->cursor.old_pos.y - x->cursor.offset.y,
				x->cursor.size.w,
				x->cursor.size.h,
				x->cursor.saved,
				0,
				0, 
				x->cursor.size.w,
				x->cursor.size.h);
	}
	else  {
		graph_blt(x->cursor.saved,
				0,
				0,
				x->cursor.size.w,
				x->cursor.size.h,
				display->g,
				x->cursor.old_pos.x - x->cursor.offset.x,
				x->cursor.old_pos.y - x->cursor.offset.y,
				x->cursor.size.w,
				x->cursor.size.h);
	}
}

static inline void refresh_cursor(x_t* x) {
	x_display_t* display = &x->displays[x->current_display];
	if(display->g == NULL || x->cursor.saved == NULL)
		return;
	int32_t mx = x->cursor.cpos.x - x->cursor.offset.x;
	int32_t my = x->cursor.cpos.y - x->cursor.offset.y;
	int32_t mw = x->cursor.saved->w;
	int32_t mh = x->cursor.saved->h;

	graph_blt(display->g, mx, my, mw, mh,
			x->cursor.saved, 0, 0, mw, mh);

	draw_cursor(display->g, &x->cursor, mx, my, x->mouse_state.busy);

	x->cursor.old_pos.x = x->cursor.cpos.x;
	x->cursor.old_pos.y = x->cursor.cpos.y;
	x->cursor.drop = false;
}

static int x_init_display(x_t* x, int32_t display_index) {
	uint32_t display_num = get_display_num(x->display_man);
	if(display_index >= 0 && display_index < (int32_t)display_num) {
		const char* fb_dev = get_display_fb_dev(x->display_man, display_index);
		if(fb_open(fb_dev, &x->displays[display_index].fb) != 0)
			return -1;
		graph_t *g_fb = fb_fetch_graph(&x->displays[display_index].fb);
		x->displays[display_index].g_fb = g_fb;
		key_t key = (((int32_t)g_fb) << 16) | proc_get_uuid(getpid());
		int32_t shm_id = shmget(key, g_fb->w * g_fb->h * 4, 0666 | IPC_CREAT | IPC_EXCL);
		if(shm_id == -1)
			return -1;
		void* p = shmat(shm_id, 0, 0);
		if(p == NULL)
			return -1;
		x->displays[display_index].g_shm_id = shm_id;
		x->displays[display_index].g = graph_new(p, g_fb->w, g_fb->h);
		x->displays[display_index].desktop_rect.x = 0;
		x->displays[display_index].desktop_rect.y = 0;
		x->displays[display_index].desktop_rect.w = g_fb->w;
		x->displays[display_index].desktop_rect.h = g_fb->h;
		
		//x_dirty(x, 0);
		x->display_num = 1;
		return 0;
	}

	for(uint32_t i=0; i<display_num; i++) {
		const char* fb_dev = get_display_fb_dev(x->display_man, i);
		if(fb_open(fb_dev, &x->displays[i].fb) != 0)
			return -1;
		graph_t *g_fb = fb_fetch_graph(&x->displays[i].fb);
		x->displays[i].g_fb = g_fb;
		key_t key = (((int32_t)g_fb) << 16) | proc_get_uuid(getpid());
		int32_t shm_id = shmget(key, g_fb->w * g_fb->h * 4, 0666 | IPC_CREAT | IPC_EXCL);
		if(shm_id == -1)
			return -1;
		void* p = shmat(shm_id, 0, 0);
		if(p == NULL)
			return -1;
		x->displays[i].g_shm_id = shm_id;
		x->displays[i].g = graph_new(p, g_fb->w, g_fb->h);
		x->displays[i].desktop_rect.x = 0;
		x->displays[i].desktop_rect.y = 0;
		x->displays[i].desktop_rect.w = g_fb->w;
		x->displays[i].desktop_rect.h = g_fb->h;
		//x_dirty(x, i);
	}
	x->display_num = display_num;
	return 0;
}

static int x_init(x_t* x, const char* display_man, int32_t display_index) {
	memset(x, 0, sizeof(x_t));
	x->xwm_pid = -1;

	x->display_man = display_man;
	if(x_init_display(x, display_index) != 0)
		return -1;

	x_display_t* display = &x->displays[0];
	x->cursor.cpos.x = display->g->w/2;
	x->cursor.cpos.y = display->g->h/2; 
	x->show_cursor = true;

	xevent_pool_init();
	return 0;
}	


static void x_close(x_t* x) {
	for(uint32_t i=0; i<x->display_num; i++) {
		x_display_t* display = &x->displays[i];
		fb_close(&display->fb);
		if(display->g != NULL) {
			shmdt(display->g->buffer);
			graph_free(display->g);
		}
		if(display->g_fb != NULL) {
			shmdt(display->g_fb->buffer);
			graph_free(display->g_fb);
		}
	}
}

static void x_repaint(x_t* x, uint32_t display_index) {
	x_display_t* display = &x->displays[display_index];
	if(display->g == NULL ||
			!display->need_repaint ||
			fb_busy(&display->fb))
		return;

	display->need_repaint = false;
	bool do_flush = false;

	if(display->cursor_task || x->mouse_state.busy) {
		display->cursor_task = false;
		do_flush = true;
	}	

	if((x->show_cursor || x->mouse_state.busy) && x->current_display == display_index)
		hide_cursor(x);

	if(display->dirty) {
		draw_desktop(x, display_index);
		do_flush = true;
	}

	xwin_t* win = x->win_head;
	while(win != NULL) {
		if(win->xinfo->visible && win->xinfo->display_index == display_index) {
			if(display->dirty || win->dirty) {
				if(draw_win(display->g, x, win) == 0)
					do_flush = true;
				if(drag_win(display->g, x, win) == 0)
					do_flush = true;
			}
		}
		win = win->next;
	}

	if(x->current_display == display_index) {
		if((x->show_cursor || x->mouse_state.busy) && check_xwm(x))
			refresh_cursor(x);
	}

	display->dirty = false;
	if(do_flush) {
		memcpy(display->g_fb->buffer,
				display->g->buffer,
				display->g->w * display->g->h * 4);
		if(x->config.gray_mode)
			graph_gray(display->g_fb);
		fb_flush(&display->fb, false);
	}
}

static xwin_t* x_get_win(x_t* x, int fd, int from_pid) {
	xwin_t* win = x->win_head;
	while(win != NULL) {
		if((win->fd == fd || fd < 0) && 
				win->from_main_pid == proc_getpid(from_pid)) {
			if(proc_check_uuid(win->from_main_pid, win->from_main_pid_uuid) == win->from_main_pid_uuid)
				return win;
			else {
				win->from_pid = -1;
				win->from_main_pid = -1;
				win->from_main_pid_uuid = 0;
			}
		}
		win = win->next;
	}
	return NULL;
}

static xwin_t* x_get_win_by_name(x_t* x, const char* name) {
	xwin_t* win = x->win_head;
	while(win != NULL) {
		if(win->xinfo->is_main &&
				strcmp(win->xinfo->name, name) == 0) {
			return win;
		}
		win = win->next;
	}
	return NULL;
}

/*
static xwin_t* get_first_visible_win(x_t* x) {
	xwin_t* ret = x->win_tail; 
	while(ret != NULL) {
		if(ret->xinfo->visible)
			return ret;
		ret = ret->prev;
	}
	return NULL;
}
*/

static void unmark_dirty(x_t* x, xwin_t* win) {
	(void)x;
	xwin_t* v = win->next;
	while(v != NULL) {
		v->dirty_mark = false;
		v = v->next;
	}
}

static void mark_dirty_confirm(x_t* x, xwin_t* win) {
	(void)x;
	xwin_t* v = win->next;
	while(v != NULL) {
		if(v->dirty_mark) {
			v->dirty = true;
			v->dirty_mark = false;
			
			if(v != win) {
				if((v->xinfo->alpha || 
						(v->xinfo->style & XWIN_STYLE_NO_FRAME) == 0 &&
						x->config.xwm_theme.alpha)) {
					x_dirty(x, v->xinfo->display_index);
				}
			}
		}
		v = v->next;
	}
}

static void mark_dirty(x_t* x, xwin_t* win) {
	if(win == NULL ||
			!win->dirty ||
			win->xinfo == NULL ||
			!win->xinfo->visible) 
		return;

	xwin_t* win_next = win->next;
	xwin_t* top = win->next;
	while(top != NULL) {
		grect_t r;
		if(top->xinfo->visible) {
			memcpy(&r, &top->xinfo->winr, sizeof(grect_t));

			grect_t *check_r;
			if(x->config.xwm_theme.alpha)
				check_r = &win->xinfo->winr;
			else
				check_r = &win->xinfo->wsr;

			grect_insect(check_r, &r);
			if(r.x == check_r->x &&
					r.y == check_r->y &&
					r.w == check_r->w &&
					r.h == check_r->h &&
					!top->xinfo->alpha) { 
				//covered by upon window. don't have to repaint.
				win->dirty = false;
				unmark_dirty(x, win);//unmark temporary dirty top win
				return;
			}
			else if(r.w > 0 && r.h > 0) {
				top->dirty_mark = true; //mark top win dirty temporary
			}
		}
		top = top->next;
	}

	mark_dirty_confirm(x, win);
	if(win->dirty)
		mark_dirty(x, win_next);
}

static void check_wins(x_t* x) {
	xwin_t* w = x->win_tail; 
	while(w != NULL) {
		xwin_t* p = w->prev;
		if(w->from_main_pid < 0 || proc_check_uuid(w->from_main_pid, w->from_main_pid_uuid) != w->from_main_pid_uuid) {
			x_del_win(x, w);
		}
		w = p;
	}
}

static void xwin_top(x_t* x, xwin_t* win) {
	remove_win(x, win);
	push_win(x, win);
}

static int do_xwin_top(int fd, int from_pid, x_t* x) {
	if(fd < 0)
		return -1;
	
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL || win->xinfo == NULL)
		return -1;
	if(!win->xinfo->visible)
		return 0;
	xwin_top(x, win);
	return 0;
}

static int do_xwin_try_focus(int fd, int from_pid, x_t* x) {
	if(fd < 0)
		return -1;
	
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL || win->xinfo == NULL)
		return -1;
	if(!win->xinfo->visible)
		return 0;

	xwin_top(x, win);
	try_focus(x, win);
	return 0;
}

static bool need_repaint_frame(x_t* x, xwin_t* win) {
	if((win->xinfo->style & XWIN_STYLE_NO_FRAME) == 0 &&
				((x->config.xwm_theme.bgEffect && !win->xinfo->focused) ||
					(x->config.xwm_theme.alpha &&
					(win->xinfo->style & XWIN_STYLE_NO_FRAME) == 0)) ||
				x->config.xwm_theme.shadow > 0)
			return true;
	return false;
}

static void win_dirty(x_t* x, xwin_t* win) {
	win->dirty = true;
	mark_dirty(x, win);
	if(win->dirty) {
		if(win->xinfo->alpha || need_repaint_frame(x, win)) {
			x_dirty(x, win->xinfo->display_index);
		}
	}
	x_repaint_req(x, win->xinfo->display_index);
}

static int x_update(int fd, int from_pid, x_t* x) {
	if(fd < 0)
		return -1;
	
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL || win->xinfo == NULL || win->ws_g == NULL)
		return -1;
	if(!win->xinfo->visible)
		return 0;
	win_dirty(x, win);	
	return 0;
}

/*
static int xwin_set_visible(int fd, int from_pid, proto_t* in, x_t* x) {
	if(fd < 0)
		return -1;
	
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL)
		return -1;

	win->xinfo->visible = proto_read_int(in);
	win->dirty = true;
	x_dirty(x, win->xinfo->display_index);
	return 0;
}
*/

static int x_update_frame_areas(x_t* x, xwin_t* win) {
	if(!check_xwm(x))
		return -1;

	if((win->xinfo->style & XWIN_STYLE_NO_FRAME) != 0)
		return -1;

	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->add(&in, win->xinfo, sizeof(xinfo_t));
	int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_FRAME_AREAS, &in, &out);
	PF->clear(&in);

	proto_read_to(&out, &win->r_title, sizeof(grect_t));
	proto_read_to(&out, &win->r_close, sizeof(grect_t));
	proto_read_to(&out, &win->r_min, sizeof(grect_t));
	proto_read_to(&out, &win->r_max, sizeof(grect_t));
	proto_read_to(&out, &win->r_resize, sizeof(grect_t));
	PF->clear(&out);
	return res;
}

static void x_get_min_size(x_t* x, xwin_t* win, int *w, int* h) {
	*w = 100;
	*h = 100;

	if(!check_xwm(x))
		return;

	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->add(&in, win->xinfo, sizeof(xinfo_t));
	int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_MIN_SIZE, &in, &out);
	PF->clear(&in);
	if(res == 0) { 
		*w = proto_read_int(&out);
		*h = proto_read_int(&out);
	}
	PF->clear(&out);
}

static int get_xwm_win_space(x_t* x, int style, grect_t* rin, grect_t* rout) {
	memcpy(rout, rin, sizeof(grect_t));
	if(!check_xwm(x))
		return 0;

	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,m", style, rin, sizeof(grect_t));

	int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_WIN_SPACE, &in, &out);
	PF->clear(&in);
	if(res == 0)
		proto_read_to(&out, rout, sizeof(grect_t));
	PF->clear(&out);

	return res;
}

enum {
	FRAME_R_TITLE = 0,
	FRAME_R_CLOSE,
	FRAME_R_MIN,
	FRAME_R_MAX,
	FRAME_R_RESIZE
};

static int get_win_frame_pos(x_t* x, xwin_t* win) {
	if((win->xinfo->style & XWIN_STYLE_NO_FRAME) != 0)
		return -1;

	int res = -1;
	if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_close))
		res = FRAME_R_CLOSE;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_min))
		res = FRAME_R_MIN;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_max))
		res = FRAME_R_MAX;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_title))
		res = FRAME_R_TITLE;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_resize))
		res = FRAME_R_RESIZE;
	return res;
}

static xwin_t* get_mouse_owner(x_t* x, int* win_frame_pos) {
	xwin_t* win = x->win_tail;
	if(win_frame_pos != NULL)
		*win_frame_pos = -1;

	while(win != NULL) {
		if(!win->xinfo->visible ||
				(win->xinfo->style & XWIN_STYLE_LAZY) != 0 ||
				win->xinfo->display_index != x->current_display) {
			win = win->prev;
			continue;
		}
		int pos = get_win_frame_pos(x, win);
		if(pos >= 0) {
			if(win_frame_pos != NULL)
				*win_frame_pos = pos;
			return win;
		}
		if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->xinfo->wsr))
			return win;
		win = win->prev;
	}
	return NULL;
}

static int x_cursor_set_busy(x_t* x, bool busy) {
	if(x->mouse_state.busy == busy)
		return 0;

	hide_cursor(x);
	if(x->cursor.saved != NULL) {
		graph_free(x->cursor.saved);
		x->cursor.saved = NULL;
	}

	x->mouse_state.busy = busy;
	if(busy && x->cursor.img_busy != NULL) {
		x->cursor.size.w = x->cursor.img_busy->w;
		x->cursor.size.h = x->cursor.img_busy->h;
		x->cursor.offset.x = x->cursor.offset_busy.x;
		x->cursor.offset.y = x->cursor.offset_busy.y;
	}
	else if(x->cursor.img != NULL) {
		x->cursor.size.w = x->cursor.img->w;
		x->cursor.size.h = x->cursor.img->h;
		x->cursor.offset.x = x->cursor.offset_normal.x;
		x->cursor.offset.y = x->cursor.offset_normal.y;
	}
	//refresh_cursor(x);
	x_repaint_req(x, x->current_display);
	return 0;
}

static int do_xwin_set_busy(int fd, int from_pid, proto_t* in, x_t* x) {
	if(fd < 0)
		return -1;

	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL)
		return -1;

	win->busy = (bool)proto_read_int(in);

	if(get_mouse_owner(x, NULL) == win)
		x_cursor_set_busy(x, win->busy);
	return 0;
}

static void mark_all_frame_dirty(x_t* x, int32_t disp_index) {
	xwin_t* w = x->win_tail; 
	while(w != NULL) {
		xwin_t* p = w->prev;
		if(w->xinfo->display_index == disp_index || disp_index < 0)
			w->frame_dirty = true; //mark dirty temporary
		w = p;
	}
	x_dirty(x, disp_index);
}

static int xwin_update_info(int fd, int from_pid, proto_t* in, proto_t* out, x_t* x) {
	if(fd < 0)
		return -1;

	int32_t xinfo_shm_id = proto_read_int(in);
	if(xinfo_shm_id == -1)
		return -1;
	uint8_t type = proto_read_int(in);
	
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL)
		return -1;

	if(win->xinfo == NULL)
		win->xinfo = shmat(xinfo_shm_id, 0, 0);
	if(win->xinfo == NULL)
		return -1;

	if((win->xinfo->style & XWIN_STYLE_LAUNCHER) != 0)
		x->win_launcher = win;
	if((win->xinfo->style & XWIN_STYLE_XIM) != 0)
		x->im_state.win_xim = win;

	int wsr_w = win->xinfo->wsr.w;
	int wsr_h = win->xinfo->wsr.h;
	
	if((win->xinfo->style & XWIN_STYLE_NO_FRAME) == 0 &&
      (win->xinfo->style & XWIN_STYLE_NO_TITLE) == 0) {
		int minw = 0, minh = 0;
		x_get_min_size(x, win, &minw, &minh);
		if(win->xinfo->wsr.w < minw)
			win->xinfo->wsr.w = minw;
		if(win->xinfo->wsr.h < minh)
			win->xinfo->wsr.h = minh;
	}

	if(win->xinfo->state == XWIN_STATE_MAX) {
		win->xinfo->wsr.x = 0;
		win->xinfo->wsr.y = x->config.xwm_theme.titleH;
		win->xinfo->wsr.w = x->displays[x->current_display].g->w;
		win->xinfo->wsr.h = x->displays[x->current_display].g->h - x->config.xwm_theme.titleH;
	}

	if(wsr_w != win->xinfo->wsr.w || wsr_h != win->xinfo->wsr.h) {
		type = type | X_UPDATE_REBUILD | X_UPDATE_REFRESH;
	}

	if(get_xwm_win_space(x, (int)win->xinfo->style,
			&win->xinfo->wsr,
			&win->xinfo->winr) != 0)	
		return -1;
	
	if((type & X_UPDATE_REBUILD) != 0 ||
			win->ws_g_shm == NULL ||
			win->frame_g_shm == NULL ||
			win->ws_g == NULL) {

		if(win->ws_g != NULL && win->ws_g_shm != NULL) {
			graph_free(win->ws_g);
			shmdt(win->ws_g_shm);
			win->ws_g = NULL;
			win->ws_g_shm = NULL;
		}

		if(win->frame_g != NULL && win->frame_g_shm != NULL) {
			graph_free(win->frame_g);
			shmdt(win->frame_g_shm);
			win->frame_g = NULL;
			win->frame_g_shm = NULL;
		}

		key_t key = (((int32_t)win) << 16) | proc_get_uuid(from_pid);
		int32_t ws_g_shm_id = shmget(key,
				win->xinfo->wsr.w * win->xinfo->wsr.h * 4,
				0666|IPC_CREAT|IPC_EXCL);
		if(ws_g_shm_id == -1)
			return -1;

		win->ws_g_shm = shmat(ws_g_shm_id, 0, 0);
		if(win->ws_g_shm == NULL) 
			return -1;

		win->xinfo->ws_g_shm_id = ws_g_shm_id;
		win->ws_g = graph_new(win->ws_g_shm, win->xinfo->wsr.w, win->xinfo->wsr.h);
		graph_clear(win->ws_g, 0x0);

		key = ((((int32_t)win) +1) << 16) | proc_get_uuid(from_pid);
		int32_t frame_g_shm_id = shmget(key,
				win->xinfo->winr.w * win->xinfo->winr.h * 4,
				0666|IPC_CREAT|IPC_EXCL);
		if(frame_g_shm_id == -1)
			return -1;

		win->frame_g_shm = shmat(frame_g_shm_id, 0, 0);
		if(win->frame_g_shm == NULL) 
			return -1;

		win->xinfo->frame_g_shm_id = frame_g_shm_id;
		win->frame_g = graph_new(win->frame_g_shm, win->xinfo->winr.w, win->xinfo->winr.h);
		//graph_clear(win->frame_g, 0x0);
		win->frame_dirty = true;
	}
	x_update_frame_areas(x, win);

	if((type & X_UPDATE_REFRESH) != 0 || win->xinfo->alpha) {
		mark_all_frame_dirty(x, win->xinfo->display_index);
	}
	return 0;
}

static int x_win_space(x_t* x, proto_t* in, proto_t* out) {
	grect_t r;
	int style = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));
	get_xwm_win_space(x, style, &r, &r); 
	PF->add(out, &r, sizeof(grect_t));
	return 0;
}

static int x_dev_load_theme(x_t* x, proto_t* in, proto_t* out) {
	PF->clear(out);
	const char* name = proto_read_str(in);
	if(name == NULL)
		return -1;
	return x_load_theme(name, &x->config.theme);
}

static int x_dev_get_theme(x_t* x, proto_t* in, proto_t* out) {
	PF->clear(out);
	PF->add(out, &x->config.theme, sizeof(x_theme_t));
	return 0;
}

static int x_dev_set_theme(x_t* x, proto_t* in, proto_t* out) {
	int32_t sz;
	x_theme_t* theme = (x_theme_t*)proto_read(in, &sz);
	if(theme == NULL || sz != sizeof(x_theme_t))
		return -1;
	memcpy(&x->config.theme, theme, sz);
	return 0;
}

static int xwm_theme_update(x_t* x) {
	if(!check_xwm(x))
		return 0;

	proto_t i;
	PF->init(&i)->add(&i, &x->config.xwm_theme, sizeof(xwm_theme_t));
	if(ipc_call(x->xwm_pid, XWM_CNTL_SET_THEME, &i, NULL) != 0) {
		return -1;
	}	
	PF->clear(&i);
	x_dirty(x, -1);
	return 0;
}

static int x_dev_load_xwm_theme(x_t* x, proto_t* in, proto_t* out) {
	PF->clear(out);
	const char* name = proto_read_str(in);
	if(name == NULL)
		return -1;

	cursor_init(name, &x->cursor);
	if(x_load_xwm_theme(name, &x->config.xwm_theme) != 0)
		return -1;
	return xwm_theme_update(x);
}

static int x_dev_get_xwm_theme(x_t* x, proto_t* in, proto_t* out) {
	PF->clear(out);
	PF->add(out, &x->config.xwm_theme, sizeof(xwm_theme_t));
	return 0;
}

static int x_dev_set_xwm_theme(x_t* x, proto_t* in, proto_t* out) {
	int32_t sz;
	xwm_theme_t* theme = (xwm_theme_t*)proto_read(in, &sz);
	if(theme == NULL || sz != sizeof(xwm_theme_t))
		return -1;
	memcpy(&x->config.xwm_theme, theme, sz);
	return xwm_theme_update(x);
}

static int x_get_desktop_space(x_t* x, proto_t* in, proto_t* out) {
	uint8_t disp_index = proto_read_int(in);

	PF->clear(out);
	if(disp_index >= DISP_MAX) {
		PF->addi(out, -1);
		return -1;
	}

	x_display_t* disp = &x->displays[disp_index];
	PF->addi(out, 0);
	PF->add(out, &disp->desktop_rect, sizeof(grect_t));
	return 0;
}

static int x_set_desktop_space(x_t* x, proto_t* in, proto_t* out) {
	uint8_t disp_index = proto_read_int(in);

	PF->clear(out);
	if(disp_index >= DISP_MAX) {
		PF->addi(out, -1);
		return -1;
	}

	x_display_t* disp = &x->displays[disp_index];
	grect_t r;
	proto_read_to(in, &r, sizeof(grect_t));
	PF->addi(out, 0);

	if(r.x == disp->desktop_rect.x &&
			r.y == disp->desktop_rect.y &&
			r.w == disp->desktop_rect.w &&
			r.h == disp->desktop_rect.h)
		return 0;
	memcpy(&disp->desktop_rect, &r, sizeof(grect_t));

	xevent_t ev;
	ev.type = XEVT_WIN;
	ev.value.window.event = XEVT_WIN_REORG;

	xwin_t* win = x->win_head;
	while(win != NULL) {
		if(win->xinfo->display_index == disp_index) {
			x_push_event(x, win, &ev);
		}
		win = win->next;
	}
	return 0;
}

static int xwin_call_xim(x_t* x, proto_t* in) {
	if(proto_read_int(in) == 0)
		hide_win(x, x->im_state.win_xim);
	else
		show_win(x, x->im_state.win_xim);
	return 0;
}

static int xserver_fcntl(int fd, int from_pid, fsinfo_t* node,
		int cmd, proto_t* in, proto_t* out, void* p) {
	(void)node;
	x_t* x = (x_t*)p;

	int res = -1;
	if(cmd == XWIN_CNTL_UPDATE) {
		res = x_update(fd, from_pid, x);
	}	
	else if(cmd == XWIN_CNTL_UPDATE_INFO) {
		res = xwin_update_info(fd, from_pid, in, out, x);
	}
	else if(cmd == XWIN_CNTL_WORK_SPACE) {
		res = x_win_space(x, in, out);
	}
	else if(cmd == XWIN_CNTL_CALL_XIM) {
		res = xwin_call_xim(x, in);
	}
	else if(cmd == XWIN_CNTL_TRY_FOCUS) {
		res = do_xwin_try_focus(fd, from_pid, x);
	}
	else if(cmd == XWIN_CNTL_TOP) {
		res = do_xwin_top(fd, from_pid, x);
	}
	else if(cmd == XWIN_CNTL_SET_BUSY) {
		res = do_xwin_set_busy(fd, from_pid, in, x);
	}
	return res;
}

static int xserver_win_open(int fd, int from_pid, fsinfo_t* node, int oflag, void* p) {
	(void)oflag;
	(void)node;
	if(fd < 0)
		return -1;

	x_t* x = (x_t*)p;
	xwin_t* win = (xwin_t*)malloc(sizeof(xwin_t));
	if(win == NULL)
		return -1;

	memset(win, 0, sizeof(xwin_t));
	win->fd = fd;
	win->from_pid = from_pid;
	win->from_main_pid = proc_getpid(from_pid);
	win->from_main_pid_uuid = proc_get_uuid(win->from_main_pid);
	push_win(x, win);
	return 0;
}

static void mouse_cxy(x_t* x, uint32_t display_index, int32_t rx, int32_t ry) {
	x_display_t* display = &x->displays[display_index];
	x->cursor.cpos.x += rx;
	x->cursor.cpos.y += ry;

	if(x->cursor.cpos.x < 0)
		x->cursor.cpos.x = 0;

	if(x->cursor.cpos.x > (int32_t)display->g->w)
		x->cursor.cpos.x = display->g->w;

	if(x->cursor.cpos.y < 0)
		x->cursor.cpos.y = 0;

	if(x->cursor.cpos.y >= (int32_t)display->g->h)
		x->cursor.cpos.y = display->g->h;
}

static void xwin_close(x_t* x, xwin_t* win) {
	if(win == NULL || win == x->win_launcher)
		return;

	xevent_t ev;
	ev.type = XEVT_WIN;
	ev.value.window.event = XEVT_WIN_CLOSE;
	x_push_event(x, win, &ev);
}
 
static void xwin_bg(x_t* x, xwin_t* win) {
	if(win == NULL)
		return;
	if(!x->config.bg_run &&
			win != x->win_launcher &&
			(win->xinfo->style & XWIN_STYLE_SYSTOP) == 0) {
		xwin_close(x, win);
		return;
	}

	if(x->win_focus != x->win_launcher) {
		x->win_last = x->win_focus;
		xwin_top(x, x->win_launcher);
	}
	else if(x->win_last != NULL) {
		xwin_top(x, x->win_last);
	}
	x_dirty(x, -1);
}

static void mouse_xwin_handle(x_t* x, xwin_t* win, int pos, xevent_t* ev) {
	if(ev->state ==  MOUSE_STATE_DOWN) {
		if(win != x->win_tail) {
			xwin_top(x, win);
		}
		else {
			try_focus(x, win);
		}
		
		if(pos == FRAME_R_TITLE) {//window title 
			x->current.win_drag = win;
			x->current.old_pos.x = x->cursor.cpos.x;
			x->current.old_pos.y = x->cursor.cpos.y;
			x->current.drag_state = X_win_DRAG_MOVE;
		}
		else if(pos == FRAME_R_RESIZE) {//window resize
			x->current.win_drag = win;
			x->current.old_pos.x = x->cursor.cpos.x;
			x->current.old_pos.y = x->cursor.cpos.y;
			x->current.drag_state = X_win_DRAG_RESIZE;
		}
	}
	else if(ev->state ==  MOUSE_STATE_DRAG) {
		if(win->xinfo->state != XWIN_STATE_MAX) {
			if(pos == FRAME_R_TITLE) {//window title 
				x->current.old_pos.x = x->cursor.cpos.x;
				x->current.old_pos.y = x->cursor.cpos.y;
				x->current.drag_state = X_win_DRAG_MOVE;
			}
			else if(pos == FRAME_R_RESIZE) {//window resize
				x->current.old_pos.x = x->cursor.cpos.x;
				x->current.old_pos.y = x->cursor.cpos.y;
				x->current.drag_state = X_win_DRAG_RESIZE;
			}
		}
	}
	else if(ev->state == MOUSE_STATE_UP) {
		if(pos == FRAME_R_RESIZE) //window resize
			return;

		if(x->current.win_drag == win &&
				x->current.drag_state != 0 &&
				win->xinfo->state != XWIN_STATE_MAX) {
			ev->type = XEVT_WIN;
			ev->value.window.v0 =  x->current.pos_delta.x;
			ev->value.window.v1 =  x->current.pos_delta.y;
			if(x->current.drag_state == X_win_DRAG_RESIZE) {
				if(x->current.pos_delta.x != 0 ||
					x->current.pos_delta.y != 0 ) {
					ev->value.window.event = XEVT_WIN_RESIZE;
					graph_free(win->ws_g);
					shmdt(win->ws_g_shm);
					win->ws_g = NULL;
					win->ws_g_shm = NULL;
				}
			}
			else if(x->current.drag_state == X_win_DRAG_MOVE) {
				ev->value.window.event = XEVT_WIN_MOVE;
			}
			x->current.pos_delta.x = 0;
			x->current.pos_delta.y = 0;
		}
		else if(abs_32(ev->value.mouse.from_x - ev->value.mouse.x) < 6 &&
				abs_32(ev->value.mouse.from_y - ev->value.mouse.y) < 6) {
			x_push_event(x, win, ev);
			ev->state = MOUSE_STATE_CLICK;
		}

		if(ev->state == MOUSE_STATE_CLICK) {
			if(pos == FRAME_R_CLOSE) { //window close
				ev->type = XEVT_WIN;
				ev->value.window.event = XEVT_WIN_CLOSE;
				//win->xinfo->visible = false;
				//x_dirty(x);
			}
			else if(pos == FRAME_R_MAX) {
				ev->type = XEVT_WIN;
				ev->value.window.event = XEVT_WIN_MAX;
			}
		}
		x->current.win_drag = NULL;
		x->current.drag_state = 0;
	}

	if(x->current.win_drag == win && x->current.drag_state != 0) {
		int mrx = x->cursor.cpos.x - x->current.old_pos.x;
		int mry = x->cursor.cpos.y - x->current.old_pos.y;
		if(abs(mrx) > 3 || abs(mry) > 3) {
			x->current.pos_delta.x = mrx;
			x->current.pos_delta.y = mry;
		}
		x_dirty(x, x->current_display);
		return; //drag win frame, don't push xwin event.
	}

	x_push_event(x, win, ev);
}

static void cursor_safe(x_t* x, x_display_t* display) {
	int margin_x = (x->cursor.size.w - x->cursor.offset.x) / 4;
	int margin_y = (x->cursor.size.h - x->cursor.offset.y) / 4;

	if(x->cursor.cpos.x < x->cursor.offset.x)
		x->cursor.cpos.x = x->cursor.offset.x;
	else if(x->cursor.cpos.x > (display->g->w - margin_x))
		x->cursor.cpos.x = display->g->w - margin_x;

	if(x->cursor.cpos.y < x->cursor.offset.y)
		x->cursor.cpos.y = x->cursor.offset.y;
	else if(x->cursor.cpos.y > (display->g->h - margin_y))
		x->cursor.cpos.y = display->g->h - margin_y;
}

static int mouse_handle(x_t* x, xevent_t* ev) {
	if(ev->value.mouse.relative != 0) {
		mouse_cxy(x, x->current_display, ev->value.mouse.rx, ev->value.mouse.ry);
		ev->value.mouse.x = x->cursor.cpos.x;
		ev->value.mouse.y = x->cursor.cpos.y;
	}
	else {
		x->cursor.cpos.x = ev->value.mouse.x;
		x->cursor.cpos.y = ev->value.mouse.y;
	}

	x_display_t *display = &x->displays[x->current_display];
	display->cursor_task = true;
	cursor_safe(x, display);
	if(ev->state ==  MOUSE_STATE_DOWN) {
		x->cursor.down = true;
		if(x->mouse_state.state == 0) {
			x->mouse_state.state = MOUSE_STATE_DOWN;
			x->mouse_state.down_pos.x = ev->value.mouse.x;
			x->mouse_state.down_pos.y = ev->value.mouse.y;
			x->mouse_state.last_pos.x = ev->value.mouse.x;
			x->mouse_state.last_pos.y = ev->value.mouse.y;
			ev->value.mouse.rx = 0;
			ev->value.mouse.ry = 0;
		}
		//else if(ev->value.mouse.from_x != ev->value.mouse.x ||
			//		ev->value.mouse.from_y != ev->value.mouse.y ||
		else if(abs(x->mouse_state.last_pos.x - ev->value.mouse.x) > 3 ||
				abs(x->mouse_state.last_pos.y - ev->value.mouse.y) > 3 ||
					x->mouse_state.state == MOUSE_STATE_DRAG) {
			x->mouse_state.state = MOUSE_STATE_DRAG;
			ev->state = MOUSE_STATE_DRAG;
			ev->value.mouse.from_x = x->mouse_state.down_pos.x;
			ev->value.mouse.from_y = x->mouse_state.down_pos.y;
			ev->value.mouse.rx = ev->value.mouse.x - x->mouse_state.last_pos.x;
			ev->value.mouse.ry = ev->value.mouse.y - x->mouse_state.last_pos.y;
			x->mouse_state.last_pos.x = ev->value.mouse.x;
			x->mouse_state.last_pos.y = ev->value.mouse.y;
		}
	}
	else if(ev->state ==  MOUSE_STATE_UP) {
		x->cursor.down = false;
		x->mouse_state.state = 0;
		ev->value.mouse.from_x = x->mouse_state.down_pos.x;
		ev->value.mouse.from_y = x->mouse_state.down_pos.y;
		ev->value.mouse.rx = ev->value.mouse.x - x->mouse_state.last_pos.x;
		ev->value.mouse.ry = ev->value.mouse.y - x->mouse_state.last_pos.y;
		x->mouse_state.last_pos.x = ev->value.mouse.x;
		x->mouse_state.last_pos.y = ev->value.mouse.y;
	}

	int pos = -1;
	xwin_t* win = NULL;
	if(x->current.win_drag != NULL)
		win = x->current.win_drag;
	else {
		win = get_mouse_owner(x, &pos);
	}

	if(win != NULL) {
		x_cursor_set_busy(x, win->busy);
		mouse_xwin_handle(x, win, pos, ev);
	}
	else {
		x_cursor_set_busy(x, false);
		if(ev->state ==  MOUSE_STATE_DOWN)
			x_unfocus(x);
	}

	return 0;
}

static int im_handle(x_t* x, int32_t from_pid, xevent_t* ev) {
	if(ev->value.im.value == KEY_HOME) {
		if(ev->state == XIM_STATE_RELEASE)
			xwin_bg(x, x->win_focus);
		return 0;
	}

	if(ev->value.im.value == KEY_END) {
		if(ev->state == XIM_STATE_RELEASE)
			xwin_close(x, x->win_focus);
		return 0;
	}

	if(ev->state == XIM_STATE_PRESS && x->win_focus)
		x->im_state.down_win_fd = x->win_focus->fd;

	if(x->im_state.win_xim_actived && x->im_state.win_xim != NULL && from_pid != x->im_state.win_xim->from_pid) {
		x_push_event(x, x->im_state.win_xim, ev);
	}
	else if(x->win_focus != NULL && x->im_state.down_win_fd == x->win_focus->fd) {
		x_push_event(x, x->win_focus, ev);
	}
	return 0;
}

static void handle_input(x_t* x, int32_t from_pid, xevent_t* ev) {
	if(ev->type == XEVT_IM) {
		im_handle(x, from_pid, ev);
	}
	else if(ev->type == XEVT_MOUSE) {
		mouse_handle(x, ev);
		x_repaint_req(x, x->current_display);
	}
}

static int x_set_top(x_t* x, const char* name, proto_t* out) {
	xwin_t* win = x_get_win_by_name(x, name);
	PF->clear(out)->addi(out, -1);
	if(win != NULL) {
		xwin_top(x, win);
		PF->clear(out)->addi(out, 0);
	}
	return 0;
}

static int xserver_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	x_t* x = (x_t*)p;

	if(cmd == DEV_CNTL_REFRESH) {
		x_dirty(x, -1);
	}
	else if(cmd == X_DCNTL_GET_INFO) {
		uint32_t i = 0;
		if(in != NULL)
			i = proto_read_int(in);

		x_display_t* display = &x->displays[i]; //TODO
		xscreen_info_t scr;	
		scr.id = 0;
		scr.fps = x->config.fps;
		scr.size.w = display->g->w;
		scr.size.h = display->g->h;
		scr.g_shm_id = display->g_shm_id;
		PF->add(ret, &scr, sizeof(xscreen_info_t));
	}
	else if(cmd == X_DCNTL_GET_DISP_NUM) {
		PF->addi(ret, x->display_num);
	}
	else if(cmd == X_DCNTL_SET_XWM) {
		x->xwm_pid = from_pid;
		x->xwm_uuid = proc_get_uuid(from_pid);
		x_dirty(x, -1);
	}
	else if(cmd == X_DCNTL_UNSET_XWM) {
		x->xwm_pid = -1;
		x_dirty(x, -1);
	}
	else if(cmd == X_DCNTL_INPUT) {
		xevent_t ev;
		proto_read_to(in, &ev, sizeof(xevent_t));
		handle_input(x, from_pid, &ev);
	}
	else if(cmd == X_DCNTL_GET_EVT) {
		x_get_event(from_pid, ret);
	}
	else if(cmd == X_DCNTL_GET_DESKTOP_SPACE) {
		x_get_desktop_space(x, in, ret);
	}
	else if(cmd == X_DCNTL_SET_DESKTOP_SPACE) {
		x_set_desktop_space(x, in, ret);
	}
	else if(cmd == X_DCNTL_GET_THEME) {
		x_dev_get_theme(x, in, ret);
	}
	else if(cmd == X_DCNTL_SET_THEME) {
		x_dev_set_theme(x, in, ret);
	}
	else if(cmd == X_DCNTL_LOAD_THEME) {
		x_dev_load_theme(x, in, ret);
	}
	else if(cmd == X_DCNTL_GET_XWM_THEME) {
		x_dev_get_xwm_theme(x, in, ret);
	}
	else if(cmd == X_DCNTL_SET_XWM_THEME) {
		x_dev_set_xwm_theme(x, in, ret);
	}
	else if(cmd == X_DCNTL_LOAD_XWM_THEME) {
		x_dev_load_xwm_theme(x, in, ret);
	}
	else if(cmd == X_DCNTL_SET_TOP) {
		const char* name = proto_read_str(in);
		x_set_top(x, name, ret);
	}
	else if(cmd == X_DCNTL_QUIT) {
		x_quit(from_pid);
	}
	return 0;
}

static int xserver_win_close(int fd, int from_pid, uint32_t node, fsinfo_t* fsinfo, void* p) {
	(void)node;
	x_t* x = (x_t*)p;
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL) {
		return -1;
	}

	if(win->busy && get_mouse_owner(x, NULL) == win)
		x_cursor_set_busy(x, false);

	int disp_index = win->xinfo->display_index;
	x_del_win(x, win);	

	x_dirty(x, disp_index);
	return 0;
}

int xserver_step(void* p) {
	x_t* x = (x_t*)p;

	uint64_t tik = kernel_tic_ms(0);
	uint32_t tm = 1000/x->config.fps;

	if(x->mouse_state.busy)
		x_repaint_req(x, x->current_display);

	ipc_disable();
	check_wins(x);

	if(core_get_active_ux() == UX_X_DEFAULT) {
		for(uint32_t i=0; i<x->display_num; i++) {
			x_repaint(x, i);
		}
	}
	else {
		x_dirty(x, -1);
	}
	ipc_enable();

	uint32_t gap = (uint32_t)(kernel_tic_ms(0) - tik);
	if(gap < tm) {
		gap = tm - gap;
		proc_usleep(gap*1000);
	}
	return 0;
}

char* xserver_dev_cmd(int from_pid, int argc, char** argv, void* p);

static int _disp_index = 0;
static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "d:");
		if(c == -1)
			break;

		switch (c) {
		case 'd':
			_disp_index = atoi(optarg);
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	const char* mnt_point = "/dev/x";
	const char* display_man = "/dev/display";
	doargs(argc, argv);

	core_set_ux(UX_X_DEFAULT);

	x_t x;
	if(x_init(&x, display_man, _disp_index) != 0)
		return -1;

	read_config(&x, "/etc/x/x.json");
	x_load_theme("default", &x.config.theme);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xserver");
	dev.fcntl = xserver_fcntl;
	dev.close = xserver_win_close;
	dev.open = xserver_win_open;
	dev.dev_cntl = xserver_dev_cntl;
	dev.cmd = xserver_dev_cmd;
	dev.loop_step = xserver_step;
	dev.extra_data = &x;

	device_run(&dev, mnt_point, FS_TYPE_CHAR | FS_TYPE_ANNOUNIMOUS, 0666);
	x_close(&x);
	return 0;
}