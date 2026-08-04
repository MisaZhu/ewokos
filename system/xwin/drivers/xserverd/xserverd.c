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

static void release_graph_shm(graph_t** g, void** shm, int32_t* shm_id) {
	if(g != NULL && *g != NULL) {
		graph_free(*g);
		*g = NULL;
	}

	if(shm != NULL && *shm != NULL) {
		shmdt(*shm);
		*shm = NULL;
	}

	if(shm_id != NULL && *shm_id != -1) {
		*shm_id = -1;
	}
}

static uint32_t _xserver_shm_seq = 1;

static int32_t xserver_alloc_shm(uint32_t salt, int32_t size, int32_t flag, key_t* out_key) {
	for(uint32_t i = 0; i < 16; i++) {
		uint32_t seq = _xserver_shm_seq++;
		key_t key = (key_t)((salt * 2654435761u) ^ (seq * 2246822519u));
		if(key == 0)
			key = (key_t)(seq | 1u);

		int32_t shm_id = shmget(key, size, flag);
		if(shm_id != -1) {
			if(out_key != NULL)
				*out_key = key;
			return shm_id;
		}
	}
	if(out_key != NULL)
		*out_key = 0;
	return -1;
}

static int32_t read_config(x_t* x, const char* fname) {
	x->config.fps = 60;

	json_var_t *conf_var = json_parse_file(fname);	

	x->config.fps = json_get_int_def(conf_var, "fps", 30);
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

static bool need_repaint_frame(x_t* x, xwin_t* win);

static void win_mark_frame_dirty(x_t* x, xwin_t* win) {
	x_display_t *display = &x->displays[win->xinfo->display_index];

	/*the background effect mixes the desktop into the frame of an unfocused
	  window, so its frame has to be built again whenever the content
	  changed. Without such an effect the frame keeps its picture.*/
	if(win->dirty && !win->xinfo->focused &&
			x->config.xwm_theme.bgEffect != 0 &&
			(win->xinfo->style & XWIN_STYLE_NO_BG_EFFECT) == 0) {
		win->frame_dirty = true;
		return;
	}

	/*a desktop repaint only invalidates the frames that blend with what is
	  below them; a frame drawn opaquely still holds a valid picture*/
	if(display->dirty && need_repaint_frame(x, win))
		win->frame_dirty = true;
}

/*the workspace area gets fully overwritten by the blt in
  prepare_win_content, so only the decoration ring around it has to be
  reset for the alpha drawing of xwm*/
static void clear_frame_ring(xwin_t* win) {
	graph_t* g = win->frame_g;
	if(g == NULL)
		return;

	grect_t bounds = {0, 0, g->w, g->h};
	grect_t ws = {win->xinfo->wsr.x - win->xinfo->winr.x,
			win->xinfo->wsr.y - win->xinfo->winr.y,
			win->xinfo->wsr.w, win->xinfo->wsr.h};

	if(!grect_insect(&bounds, &ws) ||
			(ws.x == 0 && ws.y == 0 && ws.w == g->w && ws.h == g->h)) {
		graph_clear(g, 0);
		return;
	}

	graph_set(g, 0, 0, g->w, ws.y, 0); //top
	graph_set(g, 0, ws.y + ws.h, g->w, g->h - ws.y - ws.h, 0); //bottom
	graph_set(g, 0, ws.y, ws.x, ws.h, 0); //left
	graph_set(g, ws.x + ws.w, ws.y, g->w - ws.x - ws.w, ws.h, 0); //right
}

/*ws_dmg: damaged area of the workspace, NULL means all of it*/
static void prepare_win_content(x_t* x, xwin_t* win, const grect_t* ws_dmg) {
	x_display_t *display = &x->displays[win->xinfo->display_index];
	if(display->g == NULL)
		return;

	if(win->frame_dirty) {
		clear_frame_ring(win);
		ws_dmg = NULL; //the whole frame is being rebuilt
	}

	if(win->dirty || win->frame_dirty) {
		graph_t* g = win->ws_g_buffer;
		int32_t ox = win->xinfo->wsr.x - win->xinfo->winr.x;
		int32_t oy = win->xinfo->wsr.y - win->xinfo->winr.y;
		//klog("win title: %s win->dirty: %d win->frame_dirty: %d\n", win->xinfo->title, win->dirty, win->frame_dirty);
		if(ws_dmg != NULL) {
			graph_blt(g, ws_dmg->x, ws_dmg->y, ws_dmg->w, ws_dmg->h,
					win->frame_g,
					ox + ws_dmg->x, oy + ws_dmg->y,
					ws_dmg->w, ws_dmg->h);
		}
		else {
			graph_blt(g, 0, 0, g->w, g->h,
					win->frame_g, ox, oy,
					win->xinfo->wsr.w,
					win->xinfo->wsr.h);
		}
	}

	if(!win->frame_dirty)
		return;

	if(!check_xwm(x))
		return;

	//klog("win title: %s win->frame_dirty: %d\n", win->xinfo->title, win->frame_dirty);
	proto_t in;
	PF->format(&in, "i,i,i,m",
		(ewokos_addr_t)display->g_shm_id,
		(ewokos_addr_t)display->g->w,
		(ewokos_addr_t)display->g->h,
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
		(ewokos_addr_t)display->g_shm_id,
		(ewokos_addr_t)display->g->w,
		(ewokos_addr_t)display->g->h);

	int res = ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_DESKTOP, &in);
	PF->clear(&in);
	if(res != 0)
		graph_fill_rect(display->g, 0, 0, display->g->w, display->g->h, 0xff000000);
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
		(ewokos_addr_t)display->g_shm_id,
		(ewokos_addr_t)display->g->w,
		(ewokos_addr_t)display->g->h,
		&r, sizeof(grect_t));

	if(check_xwm(xp))
		ipc_call_wait(xp->xwm_pid, XWM_CNTL_DRAW_DRAG_FRAME, &in);
	PF->clear(&in);
}

/*blit the part of area that falls inside dmg: dmg and area are in frame
  coordinates, the result lands at win_x/win_y on the display*/
static inline void blit_win_area(graph_t* g, graph_t* disp_g, int32_t win_x, int32_t win_y,
		const grect_t* dmg, const grect_t* area, bool alpha) {
	grect_t d = *dmg;
	if(!grect_insect(area, &d) || d.w <= 0 || d.h <= 0)
		return;

	if(alpha)
		graph_blt_alpha(g, d.x, d.y, d.w, d.h,
				disp_g, win_x + d.x, win_y + d.y, d.w, d.h, 0xff);
	else
		graph_blt(g, d.x, d.y, d.w, d.h,
				disp_g, win_x + d.x, win_y + d.y, d.w, d.h);
}

/*out_dmg gets the area of disp_g the window actually touched*/
static int draw_win(graph_t* disp_g, x_t* x, xwin_t* win, grect_t* out_dmg) {
	win_mark_frame_dirty(x, win);

	grect_t ws_dmg = win->damage;
	bool has_dmg = win->has_damage && !win->frame_dirty;

	prepare_win_content(x, win, has_dmg ? &ws_dmg : NULL);

	grect_t dmg; //damaged area in frame_g coordinates
	if(has_dmg) {
		dmg.x = ws_dmg.x + win->xinfo->wsr.x - win->xinfo->winr.x;
		dmg.y = ws_dmg.y + win->xinfo->wsr.y - win->xinfo->winr.y;
		dmg.w = ws_dmg.w;
		dmg.h = ws_dmg.h;
	}
	else {
		dmg.x = 0;
		dmg.y = 0;
		dmg.w = win->xinfo->winr.w;
		dmg.h = win->xinfo->winr.h;
	}

	graph_t* g = win->frame_g;
	if(g != NULL) {
		grect_t bounds = {0, 0, g->w, g->h};
		if(!grect_insect(&bounds, &dmg)) {
			dmg.w = 0;
			dmg.h = 0;
		}
		else if(win->xinfo->alpha) {
			/*the window content itself is translucent, blend the whole window*/
			graph_blt_alpha(g, dmg.x, dmg.y, dmg.w, dmg.h,
					disp_g,
					win->xinfo->winr.x + dmg.x,
					win->xinfo->winr.y + dmg.y,
					dmg.w, dmg.h, 0xff);
		}
		else if(x->config.xwm_theme.shadow > 0 &&
				!x->config.xwm_theme.alpha) {
			/*everything is opaque except the shadow, and the shadow only
			  lives in the strips right of and below the workspace: copy the
			  rest with a plain blit and blend just those strips*/
			int32_t s_right = win->xinfo->winr.w -
					((win->xinfo->wsr.x - win->xinfo->winr.x) + win->xinfo->wsr.w);
			int32_t s_bottom = win->xinfo->winr.h -
					((win->xinfo->wsr.y - win->xinfo->winr.y) + win->xinfo->wsr.h);
			if(s_right < 0)
				s_right = 0;
			if(s_bottom < 0)
				s_bottom = 0;

			grect_t inner = {0, 0, g->w - s_right, g->h - s_bottom};
			grect_t right = {g->w - s_right, 0, s_right, g->h};
			grect_t bottom = {0, g->h - s_bottom, g->w - s_right, s_bottom};

			blit_win_area(g, disp_g, win->xinfo->winr.x, win->xinfo->winr.y,
					&dmg, &inner, false);

			/*the bands already sit blended on the display: blending them
			  again would put shadow on shadow and darken them further, so
			  they are only touched again once what is below them was
			  repainted (which invalidates the flag) or the window moved*/
			bool bands_ok = win->shadow_valid &&
					memcmp(&win->shadow_rect, &win->xinfo->winr, sizeof(grect_t)) == 0;
			if(!bands_ok) {
				/*the bands are missing on the display, so they get blended
				  whole no matter what the damaged area of this draw is*/
				grect_t whole = {0, 0, g->w, g->h};
				blit_win_area(g, disp_g, win->xinfo->winr.x, win->xinfo->winr.y,
						&whole, &right, true);
				blit_win_area(g, disp_g, win->xinfo->winr.x, win->xinfo->winr.y,
						&whole, &bottom, true);
				win->shadow_valid = true;
				memcpy(&win->shadow_rect, &win->xinfo->winr, sizeof(grect_t));
			}
		}
		else if(x->config.xwm_theme.alpha) {
			/*the frame's translucent pixels are confined to the four corner
			  squares the rounded corners are cut out of: blend just those
			  and copy everything else with a plain blit. c is the square
			  size, the radius plus the outermost ring the arc is drawn on*/
			int32_t c = 0;
			if(x->config.xwm_theme.round > 0)
				c = (int32_t)x->config.xwm_theme.round + 1;
			if(c > 0) {
				if(2*c > g->w)
					c = g->w/2;
				if(2*c > g->h)
					c = g->h/2;
			}

			if(c > 0) {
				int32_t wx = win->xinfo->winr.x;
				int32_t wy = win->xinfo->winr.y;

				grect_t c_tl = {0, 0, c, c};
				grect_t c_tr = {g->w - c, 0, c, c};
				grect_t c_bl = {0, g->h - c, c, c};
				grect_t c_br = {g->w - c, g->h - c, c, c};
				blit_win_area(g, disp_g, wx, wy, &dmg, &c_tl, true);
				blit_win_area(g, disp_g, wx, wy, &dmg, &c_tr, true);
				blit_win_area(g, disp_g, wx, wy, &dmg, &c_bl, true);
				blit_win_area(g, disp_g, wx, wy, &dmg, &c_br, true);

				/*the rest of the frame is opaque: the strips between the
				  corner squares and the big middle part*/
				grect_t top = {c, 0, g->w - 2*c, c};
				grect_t mid = {0, c, g->w, g->h - 2*c};
				grect_t bot = {c, g->h - c, g->w - 2*c, c};
				blit_win_area(g, disp_g, wx, wy, &dmg, &top, false);
				blit_win_area(g, disp_g, wx, wy, &dmg, &mid, false);
				blit_win_area(g, disp_g, wx, wy, &dmg, &bot, false);
			}
			else {
				/*alpha but no rounded corners: the translucent pixels could
				  be anywhere, blend the whole window*/
				graph_blt_alpha(g, dmg.x, dmg.y, dmg.w, dmg.h,
						disp_g,
						win->xinfo->winr.x + dmg.x,
						win->xinfo->winr.y + dmg.y,
						dmg.w, dmg.h, 0xff);
			}
		}
		else {
			graph_blt(g, dmg.x, dmg.y, dmg.w, dmg.h,
					disp_g,
					win->xinfo->winr.x + dmg.x,
					win->xinfo->winr.y + dmg.y,
					dmg.w, dmg.h);
		}
	}

	out_dmg->x = win->xinfo->winr.x + dmg.x;
	out_dmg->y = win->xinfo->winr.y + dmg.y;
	out_dmg->w = dmg.w;
	out_dmg->h = dmg.h;

	win->dirty = false;
	win->frame_dirty = false;
	win->has_damage = false;
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

static void x_get_event_node(int from_pid, proto_t* out) {
	PF->addi(out, xevent_get_node(from_pid));
}

static void x_push_event(x_t* x, xwin_t* win, xevent_t* e) {
	(void)x;
	if(win == NULL || win->from_pid <= 0 || win->xinfo == NULL)
		return;
	e->win = win->xinfo->win;
	xevent_push(win->from_pid, e);
	uint32_t evt_node = xevent_get_node(win->from_pid);
	if(evt_node != 0)
		vfs_wakeup(evt_node, VFS_EVT_RD);
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
	if(display_index >= 0 && display_index < DISP_MAX) {
		x_display_t *display = &x->displays[display_index];
		display->need_repaint = true;
		return;
	}

	for(uint32_t i=0; i<x->display_num && i<DISP_MAX; i++) {
		x_display_t *display = &x->displays[i];
		display->need_repaint = true;
	}
}

#define X_REPAINT_DIRTY_MAX 16

static inline bool rect_is_valid(const grect_t* r) {
	return r->w > 0 && r->h > 0;
}

static bool rect_clip_to_graph(graph_t* g, grect_t* r) {
	grect_t bounds = {0, 0, g->w, g->h};
	return grect_insect(&bounds, r);
}

static inline bool rect_overlap_or_touch(const grect_t* a, const grect_t* b) {
	if((a->x + a->w) < b->x || (b->x + b->w) < a->x)
		return false;
	if((a->y + a->h) < b->y || (b->y + b->h) < a->y)
		return false;
	return true;
}

static void rect_union_to(grect_t* dst, const grect_t* src) {
	int32_t x0 = dst->x < src->x ? dst->x : src->x;
	int32_t y0 = dst->y < src->y ? dst->y : src->y;
	int32_t x1 = (dst->x + dst->w) > (src->x + src->w) ? (dst->x + dst->w) : (src->x + src->w);
	int32_t y1 = (dst->y + dst->h) > (src->y + src->h) ? (dst->y + dst->h) : (src->y + src->h);
	dst->x = x0;
	dst->y = y0;
	dst->w = x1 - x0;
	dst->h = y1 - y0;
}

static inline bool rect_contains_rect(const grect_t* out, const grect_t* in) {
	return in->x >= out->x && in->y >= out->y &&
			(in->x + in->w) <= (out->x + out->w) &&
			(in->y + in->h) <= (out->y + out->h);
}

/*a region was just repainted fresh; where it reaches into the shadow bands
  of the windows above, their shadow has been wiped away or sits blended on
  top of stale pixels. Repair exactly those parts while the background is
  still pristine: re-blending a whole band later would double the shadow on
  the parts that were left alone. Not needed while the whole display is
  being rebuilt: that pass repaints every window bottom to top, so each one
  blends its bands onto a fresh background itself.*/
static void refresh_shadows_above(x_t* x, xwin_t* below, const grect_t* region) {
	x_display_t* display = &x->displays[below->xinfo->display_index];
	xwin_t* w = below->next;
	while(w != NULL) {
		if(w->ready && w->xinfo != NULL && w->xinfo->visible &&
				w->xinfo->display_index == below->xinfo->display_index &&
				w->frame_g != NULL) {
			int32_t s_right = w->xinfo->winr.w -
					((w->xinfo->wsr.x - w->xinfo->winr.x) + w->xinfo->wsr.w);
			int32_t s_bottom = w->xinfo->winr.h -
					((w->xinfo->wsr.y - w->xinfo->winr.y) + w->xinfo->wsr.h);
			if(s_right < 0)
				s_right = 0;
			if(s_bottom < 0)
				s_bottom = 0;

			if(s_right > 0 || s_bottom > 0) {
				bool bands_ok = w->shadow_valid &&
						memcmp(&w->shadow_rect, &w->xinfo->winr, sizeof(grect_t)) == 0;
				if(bands_ok) {
					/*the untouched parts of the bands are still fine: fix
					  only what the fresh region wiped*/
					grect_t d = *region;
					d.x -= w->xinfo->winr.x;
					d.y -= w->xinfo->winr.y;
					grect_t right = {w->frame_g->w - s_right, 0, s_right, w->frame_g->h};
					grect_t bottom = {0, w->frame_g->h - s_bottom,
							w->frame_g->w - s_right, s_bottom};
					blit_win_area(w->frame_g, display->g,
							w->xinfo->winr.x, w->xinfo->winr.y,
							&d, &right, true);
					blit_win_area(w->frame_g, display->g,
							w->xinfo->winr.x, w->xinfo->winr.y,
							&d, &bottom, true);
				}
				else {
					/*the bands were never blended for this placement: the
					  window has to paint them whole on its own draw*/
					w->dirty = true;
					w->has_damage = false;
				}
			}
		}
		w = w->next;
	}
}

static void x_repaint_add_dirty(graph_t* g, grect_t* rects, uint32_t* num, const grect_t* r) {
	grect_t dirty = *r;
	if(!rect_is_valid(&dirty) || !rect_clip_to_graph(g, &dirty))
		return;

	for(uint32_t i = 0; i < *num; i++) {
		if(rect_contains_rect(&rects[i], &dirty))
			return;
		if(rect_overlap_or_touch(&rects[i], &dirty)) {
			rect_union_to(&rects[i], &dirty);
			uint32_t j = 0;
			while(j < *num) {
				if(i == j) {
					j++;
					continue;
				}
				if(rect_overlap_or_touch(&rects[i], &rects[j])) {
					rect_union_to(&rects[i], &rects[j]);
					rects[j] = rects[*num - 1];
					(*num)--;
					j = 0;
					continue;
				}
				j++;
			}
			return;
		}
	}

	if(*num < X_REPAINT_DIRTY_MAX) {
		rects[*num] = dirty;
		(*num)++;
		return;
	}

	rect_union_to(&rects[0], &dirty);
}

/*the fb control block only carries FB_DIRTY_MAX rects, so merge the
  cheapest pairs together until they fit; without this the daemon has to
  push the whole framebuffer to the panel*/
static uint32_t pack_dirty_rects(const grect_t* rects, uint32_t num,
		grect_t* out, uint32_t max) {
	if(num <= max) {
		memcpy(out, rects, num * sizeof(grect_t));
		return num;
	}

	memcpy(out, rects, max * sizeof(grect_t));
	for(uint32_t i = max; i < num; i++) {
		uint32_t best = 0;
		int64_t best_cost = -1;
		for(uint32_t j = 0; j < max; j++) {
			grect_t u = out[j];
			rect_union_to(&u, &rects[i]);
			int64_t cost = (int64_t)u.w * u.h - (int64_t)out[j].w * out[j].h;
			if(best_cost < 0 || cost < best_cost) {
				best_cost = cost;
				best = j;
			}
		}
		rect_union_to(&out[best], &rects[i]);
	}
	return max;
}

static inline bool rect_contains(const grect_t* out, const grect_t* in) {
	return in->x >= out->x && in->y >= out->y &&
			(in->x + in->w) <= (out->x + out->w) &&
			(in->y + in->h) <= (out->y + out->h);
}

/* whether rect r is fully covered by the opaque workspace of one window
   above 'from' (from==NULL means checking against all windows).
   opaque means: no per-win alpha, no alpha theme, and no translucent
   bg-effect (same rule as mark_dirty) */
static bool covered_by_opaque_win(x_t* x, xwin_t* from, uint32_t display_index, const grect_t* r) {
	xwin_t* top = (from == NULL) ? x->win_head : from->next;
	while(top != NULL) {
		if(top->ready && top->xinfo != NULL && top->xinfo->visible &&
				top->xinfo->display_index == display_index &&
				!top->xinfo->alpha &&
				/*the theme cuts translucent corners into the workspace, so
				  such a window does not fully paint even its wsr*/
				!x->config.xwm_theme.alpha &&
				(top->xinfo->focused ||
				(top->xinfo->style & XWIN_STYLE_NO_BG_EFFECT) != 0)) {
			/*a shadow hangs outside the wsr and stays translucent, but the
			  wsr itself is what is tested here*/
			if(rect_contains(&top->xinfo->wsr, r))
				return true;
		}
		top = top->next;
	}
	return false;
}

static inline void x_get_cursor_rect(x_t* x, grect_t* r, bool old_pos) {
	int32_t cx = old_pos ? x->cursor.old_pos.x : x->cursor.cpos.x;
	int32_t cy = old_pos ? x->cursor.old_pos.y : x->cursor.cpos.y;
	r->x = cx - x->cursor.offset.x;
	r->y = cy - x->cursor.offset.y;
	r->w = x->cursor.size.w;
	r->h = x->cursor.size.h;
}

static void push_win(x_t* x, xwin_t* win) {
	if(win->xinfo == NULL) { //xinfo not ready, just add to head
		if(x->win_head != NULL) {
			x->win_head->prev = win;
			win->next = x->win_head;
			x->win_head = win;
		}
		else {
			x->win_tail = x->win_head = win;
		}
		return;
	}

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
			if(win_top->xinfo == NULL) {
				win_top = win_top->prev;
				continue;
			}
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

static xwin_t* get_top_focus_win(x_t* x, bool skip_launcher) {
	xwin_t* ret = x->win_tail; 
	while(ret != NULL) {
		if(ret->xinfo != NULL && ret->xinfo->visible &&
				(ret->xinfo->style & XWIN_STYLE_NO_FOCUS) == 0 &&
				(!skip_launcher || ret != x->win_launcher))
			return ret;
		ret = ret->prev;
	}
	return NULL;
}

static xwin_t* get_next_focus_win(x_t* x, bool skip_launcher) {
	xwin_t* ret = x->win_head;
	while(ret != NULL) {
		if(ret->xinfo != NULL && ret->xinfo->visible &&
				(ret->xinfo->style & XWIN_STYLE_NO_FOCUS) == 0 &&
				(!skip_launcher || ret != x->win_launcher))
			return ret;
		ret = ret->next;
	}
	return NULL;
}

static void x_del_win(x_t* x, xwin_t* win) {
	if(win == x->win_focus)
		hide_win(x, x->im_state.win_xim);

	remove_win(x, win);
	if(win == x->current.win_drag) {
		x->current.win_drag = NULL;
		x->current.drag_state = 0;
		x->current.pos_delta.x = 0;
		x->current.pos_delta.y = 0;
	}
	if(win == x->im_state.win_xim) {
		x->im_state.win_xim = NULL;
		x->im_state.win_xim_actived = false;
	}
	if(x->im_state.down_win_fd == win->fd)
		x->im_state.down_win_fd = -1;
	if(win == x->win_launcher)
		x->win_launcher = NULL;
	if(win == x->win_last)
		x->win_last = NULL;

	if(win->xinfo != NULL) {
		release_graph_shm(&win->ws_g, &win->ws_g_shm, &win->xinfo->ws_g_shm_id);
		release_graph_shm(&win->frame_g, &win->frame_g_shm, &win->xinfo->frame_g_shm_id);
	}
	
	if(win->ws_g_buffer != NULL) {
		graph_free(win->ws_g_buffer);
		win->ws_g_buffer = NULL;
	}

	shmdt(win->xinfo);
	if(win == x->win_focus)
		x->win_focus = NULL;
	free(win);
	win = get_top_focus_win(x, false);
	x->win_last = get_top_focus_win(x, true);

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
		if(display_fb_open(x->display_man, display_index, &x->displays[display_index].fb) != 0)
			return -1;
		graph_t *g_fb = fb_fetch_graph(&x->displays[display_index].fb);
		if(g_fb == NULL)
			return -1;
		/* Composite straight into the scan-out dma: no shadow buffer and
		   no per-frame copy back into fb. */
		x->displays[display_index].g_fb = g_fb;
		x->displays[display_index].g = g_fb;
		x->displays[display_index].g_shm_id = x->displays[display_index].fb.dma_id;
		x->displays[display_index].desktop_rect.x = 0;
		x->displays[display_index].desktop_rect.y = 0;
		x->displays[display_index].desktop_rect.w = g_fb->w;
		x->displays[display_index].desktop_rect.h = g_fb->h;

		//x_dirty(x, 0);
		x->display_num = 1;
		return 0;
	}

	for(uint32_t i=0; i<display_num; i++) {
		if(display_fb_open(x->display_man, i, &x->displays[i].fb) != 0)
			return -1;
		graph_t *g_fb = fb_fetch_graph(&x->displays[i].fb);
		if(g_fb == NULL)
			return -1;
		x->displays[i].g_fb = g_fb;
		x->displays[i].g = g_fb;
		x->displays[i].g_shm_id = x->displays[i].fb.dma_id;
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
	for(uint32_t i = 0; i < DISP_MAX; i++) {
		x->displays[i].g_shm_id = -1;
	}

	x->display_man = display_man;
	if(x_init_display(x, display_index) != 0)
		return -1;

	x_display_t* display = &x->displays[0];
	x->cursor.cpos.x = display->g->w/2;
	x->cursor.cpos.y = display->g->h/2;
	x->mouse_state.last_pos.x = x->cursor.cpos.x;
	x->mouse_state.last_pos.y = x->cursor.cpos.y;
	x->show_cursor = true;

	xevent_pool_init();
	return 0;
}	


static void x_close(x_t* x) {
	for(uint32_t i=0; i<x->display_num; i++) {
		x_display_t* display = &x->displays[i];
		/* g and g_fb both alias the fb dma graph here; fb_close frees it. */
		fb_close(&display->fb);
		display->g = NULL;
		display->g_fb = NULL;
		display->g_shm_id = -1;
	}
}

static bool all_win_ready(x_t* x) {
	xwin_t* win = x->win_head;
	while(win != NULL) {
		if(win->xinfo != NULL && win->xinfo->visible && !win->ready)
			return false;
		win = win->next;
	}
	return true;
}

static bool x_is_hide_cursor_on_win(x_t* x) {
	xwin_t* win = get_top_focus_win(x, false);
	if(win == NULL)
		return false;
	if(!win->xinfo->hide_cursor)
		return false;

	int32_t mx = x->cursor.cpos.x;
	int32_t my = x->cursor.cpos.y;

	if(mx < win->xinfo->wsr.x || mx > (win->xinfo->wsr.x + win->xinfo->wsr.w) ||
			my < win->xinfo->wsr.y || my > (win->xinfo->wsr.y + win->xinfo->wsr.h))
		return false;
	return true;
}

#define X_WAIT_READY_MAX 4

static void x_repaint(x_t* x, uint32_t display_index) {
	x_display_t* display = &x->displays[display_index];
	grect_t dirty_rects[X_REPAINT_DIRTY_MAX];
	uint32_t dirty_num = 0;
	bool cursor_hidden = false;
	bool cursor_refreshed = false;
	grect_t cursor_old_rect;
	grect_t cursor_new_rect;

	if(display->g == NULL || !display->need_repaint)
		return;

	/*compositing writes straight into the scan-out dma now, so hold off
	  while the fb daemon is still pushing the previous frame to the panel;
	  otherwise it copies a half-drawn frame (tearing). need_repaint stays
	  set so the frame is retried on the next step.*/
	if(fb_busy(&display->fb))
		return;

	if(!all_win_ready(x)) {
		/*wait a few frames only: a client stuck before its first update
		  must not freeze the whole display*/
		if(display->wait_ready < X_WAIT_READY_MAX) {
			display->wait_ready++;
			return;
		}
	}
	display->wait_ready = 0;

	display->need_repaint = false;
	bool do_flush = false;

	if(display->cursor_task || x->mouse_state.busy) {
		display->cursor_task = false;
		do_flush = true;
	}	

	if((x->show_cursor || x->mouse_state.busy) && x->current_display == display_index) {
		if(!x_is_hide_cursor_on_win(x)) {
			x_get_cursor_rect(x, &cursor_old_rect, true);
			hide_cursor(x);
			cursor_hidden = true;
		}
	}

	if(display->dirty) {
		/* skip desktop drawing if fully covered by an opaque window
		   (e.g. fullscreen window on top) */
		if(!covered_by_opaque_win(x, NULL, display_index, &display->desktop_rect)) {
			draw_desktop(x, display_index);
			x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &display->desktop_rect);
			do_flush = true;
		}
	}

	xwin_t* win = x->win_head;
	while(win != NULL) {
		if(win->ready && 
				win->xinfo->visible &&
				win->xinfo->display_index == display_index) {
			if(display->dirty) {
				win->dirty = true;
				win->has_damage = false; //everything below it got repainted
				win->shadow_valid = false; //the bands must be blended fresh
			}

			if(win->dirty || win->frame_dirty) {
				/* fully covered by an opaque window above: the covering
				   window paints over it later in this bottom-to-top pass,
				   so drawing it would be pure waste */
				if(win != x->current.win_drag &&
								covered_by_opaque_win(x, win, display_index, &win->xinfo->winr)) {
					win->dirty = false;
					win->frame_dirty = false;
					win->has_damage = false;
					/*its area gets overwritten by the covering window, so
					  whatever shadow sat there is gone*/
					win->shadow_valid = false;
				}
				else {
					grect_t win_dirty;
					if(draw_win(display->g, x, win, &win_dirty) == 0) {
						if(!display->dirty)
							refresh_shadows_above(x, win, &win_dirty);
						x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &win_dirty);
						do_flush = true;
					}
					if(drag_win(display->g, x, win) == 0) {
						x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &display->desktop_rect);
						do_flush = true;
					}
				}
			}
		}
		win = win->next;
	}

	if(x->current_display == display_index) {
		if(x->show_cursor || x->mouse_state.busy) {
			if(!x_is_hide_cursor_on_win(x)) {
				x_get_cursor_rect(x, &cursor_new_rect, false);
				refresh_cursor(x);
				cursor_refreshed = true;
			}
		}
	}

	if(cursor_hidden) {
		x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &cursor_old_rect);
	}
	if(cursor_refreshed) {
		x_repaint_add_dirty(display->g, dirty_rects, &dirty_num, &cursor_new_rect);
	}

	display->dirty = false;
	if(do_flush && dirty_num > 0) {
		/*tell the fb daemon what changed so it only pushes those areas to
		  the scan-out buffer instead of the whole frame*/
		grect_t fb_dirty[FB_DIRTY_MAX];
		uint32_t fb_num = pack_dirty_rects(dirty_rects, dirty_num, fb_dirty, FB_DIRTY_MAX);
		fb_set_dirty(&display->fb, fb_dirty, fb_num);
		/*defer the flush IPC until after ipc_enable() to keep the
		  ipc_disable() critical section as tight as possible*/
		display->pending_flush = true;
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

static bool has_win_by_main_pid(x_t* x, int main_pid) {
	xwin_t* win = x->win_head;
	while(win != NULL) {
		if(win->from_main_pid == main_pid)
			return true;
		win = win->next;
	}
	return false;
}

static xwin_t* x_get_win_by_name(x_t* x, const char* name) {
	xwin_t* win = x->win_head;
	while(win != NULL) {
		if(win->xinfo != NULL && win->xinfo->is_main &&
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
			v->has_damage = false; //the area below it was repainted
			/*no need for a whole display repaint: every window above
			  the dirty one that intersects it got marked here and is
			  redrawn bottom to top in the same pass, so each blends
			  onto content that was just made fresh*/
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
		if(top->xinfo != NULL && top->xinfo->visible) {
			memcpy(&r, &top->xinfo->winr, sizeof(grect_t));

			grect_t *check_r;
			if(x->config.xwm_theme.alpha || x->config.xwm_theme.shadow > 0)
				check_r = &win->xinfo->winr;
			else
				check_r = &win->xinfo->wsr;

			grect_insect(check_r, &r);
			if(r.w > 0 && r.h > 0)
				top->dirty_mark = true; //mark top win dirty temporary
			
			if(r.x == check_r->x &&
					r.y == check_r->y &&
					r.w == check_r->w &&
					r.h == check_r->h) {
				if(!top->xinfo->alpha && 
					(top->xinfo->focused ||
					(top->xinfo->style & XWIN_STYLE_NO_BG_EFFECT) != 0)) { 
					//covered by upon window. don't have to repaint.
						win->dirty = false;
						unmark_dirty(x, win);//unmark temporary dirty top win
						return;
				}
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
	if(win == x->win_focus)
		return;
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
	if((win->xinfo->style & XWIN_STYLE_NO_FRAME) != 0 && !win->xinfo->alpha)
		return false;

	if(x->config.xwm_theme.shadow > 0 ||
		(x->config.xwm_theme.bgEffect && !win->xinfo->focused) ||
		x->config.xwm_theme.alpha)
			return true;
	return false;
}

static void win_dirty(x_t* x, xwin_t* win) {
	win->dirty = true;
	mark_dirty(x, win);
	if(win->dirty) {
		/*only content that reads the desktop back has to repaint the whole
		  display; a shadow is blended from the frame at blit time, so it is
		  fine with the window simply being redrawn*/
		if(win->xinfo->alpha ||
				(x->config.xwm_theme.alpha && need_repaint_frame(x, win))) {
			x_dirty(x, win->xinfo->display_index);
		}
	}
	x_repaint_req(x, win->xinfo->display_index);
}

#define X_DAMAGE_BAND     8
#define X_DAMAGE_SKIP_MAX 8

/*compare the workspace against the copy we keep and return the bounding box
  of what the client actually changed. The rows are checked in bands so a
  single memcmp covers several of them; the columns are then only probed
  where they could still widen the box. Returns false when nothing changed.*/
static bool detect_ws_damage(xwin_t* win, grect_t* dmg) {
	const uint32_t* src = win->ws_g->buffer;
	const uint32_t* dst = win->ws_g_buffer->buffer;
	int32_t w = win->ws_g->w;
	int32_t h = win->ws_g->h;
	size_t row_bytes = (size_t)w * sizeof(uint32_t);

	int32_t y0 = -1, y1 = 0;
	int32_t x0 = w, x1 = 0;

	for(int32_t y = 0; y < h; y += X_DAMAGE_BAND) {
		int32_t bh = (y + X_DAMAGE_BAND) > h ? (h - y) : X_DAMAGE_BAND;
		const uint32_t* s = src + (size_t)y * w;
		const uint32_t* d = dst + (size_t)y * w;
		if(memcmp(s, d, row_bytes * bh) == 0)
			continue;

		if(y0 < 0)
			y0 = y;
		y1 = y + bh;

		if(x0 == 0 && x1 == w) //already full width, only the rows still matter
			continue;

		for(int32_t i = 0; i < bh; i++) {
			const uint32_t* sr = s + (size_t)i * w;
			const uint32_t* dr = d + (size_t)i * w;
			int32_t l = 0;
			while(l < x0 && sr[l] == dr[l])
				l++;
			if(l < x0)
				x0 = l;

			int32_t r = w - 1;
			while(r >= x1 && sr[r] == dr[r])
				r--;
			if(r >= x1)
				x1 = r + 1;

			if(x0 == 0 && x1 == w)
				break;
		}
	}

	if(y0 < 0 || x1 <= x0)
		return false;

	dmg->x = x0;
	dmg->y = y0;
	dmg->w = x1 - x0;
	dmg->h = y1 - y0;
	return true;
}

static void copy_ws_rect(xwin_t* win, const grect_t* r) {
	graph_t* src = win->ws_g;
	graph_t* dst = win->ws_g_buffer;
	size_t row_bytes = (size_t)r->w * sizeof(uint32_t);
	for(int32_t y = 0; y < r->h; y++) {
		memcpy(dst->buffer + (size_t)(r->y + y) * dst->w + r->x,
				src->buffer + (size_t)(r->y + y) * src->w + r->x,
				row_bytes);
	}
}

static int x_update(int fd, int from_pid, x_t* x) {
	if(fd < 0)
		return -1;
	
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL || win->xinfo == NULL ||
			win->ws_g == NULL || win->ws_g_buffer == NULL)
		return -1;

	if(!win->xinfo->visible)
		return 0;

	grect_t full = {0, 0, win->ws_g->w, win->ws_g->h};
	grect_t dmg = full;
	bool has_dmg = false;

	if(win->ready && win->damage_skip == 0) {
		has_dmg = detect_ws_damage(win, &dmg);
		if(!has_dmg) {
			if(!win->dirty && !win->frame_dirty)
				return 0; //the client redrew the very same picture
			/*nothing new in the workspace, but a repaint is still pending:
			  keep the damage already recorded and just ask for it again*/
			win_dirty(x, win);
			return 0;
		}

		/*detection costs a full compare pass: back off for a while when the
		  client keeps repainting almost everything anyway*/
		if((int64_t)dmg.w * dmg.h * 4 >= (int64_t)full.w * full.h * 3)
			win->damage_skip = X_DAMAGE_SKIP_MAX;
	}
	else if(win->damage_skip > 0) {
		win->damage_skip--;
	}

	/*several updates may pile up between two repaints, so never narrow a
	  damage that has not been composited yet*/
	if(win->dirty && !win->has_damage)
		has_dmg = false;
	else if(has_dmg && win->has_damage)
		rect_union_to(&dmg, &win->damage);

	if(has_dmg) {
		copy_ws_rect(win, &dmg);
	}
	else {
		dmg = full;
		memcpy(win->ws_g_buffer->buffer, win->ws_g->buffer,
				(size_t)win->ws_g->w * win->ws_g->h * sizeof(uint32_t));
	}

	win->damage = dmg;
	win->has_damage = has_dmg;
	win->ready = true;
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

static int get_xwm_win_space(x_t* x, int style, int state, grect_t* rin, grect_t* rout) {
	memcpy(rout, rin, sizeof(grect_t));
	if(!check_xwm(x))
		return 0;

	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,i,m", (ewokos_addr_t)style, (ewokos_addr_t)state, rin, sizeof(grect_t));

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
		if(win->xinfo == NULL ||
				!win->xinfo->visible ||
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
		if(w->xinfo != NULL &&
				(w->xinfo->display_index == (uint32_t)disp_index || disp_index < 0))
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
	if(win->xinfo == (void*)-1) {
		win->xinfo = NULL;
		return -1;
	}
	if(win->xinfo->ws_g_shm_id == 0 && win->ws_g_shm == NULL)
		win->xinfo->ws_g_shm_id = -1;
	if(win->xinfo->frame_g_shm_id == 0 && win->frame_g_shm == NULL)
		win->xinfo->frame_g_shm_id = -1;

	if((win->xinfo->style & XWIN_STYLE_LAUNCHER) != 0)
		x->win_launcher = win;
	if((win->xinfo->style & XWIN_STYLE_XIM) != 0)
		x->im_state.win_xim = win;

	int wsr_w = win->xinfo->wsr.w;
	int wsr_h = win->xinfo->wsr.h;
	int winr_w = win->xinfo->winr.w;
	int winr_h = win->xinfo->winr.h;
	
	if((win->xinfo->style & XWIN_STYLE_NO_FRAME) == 0 &&
      (win->xinfo->style & XWIN_STYLE_NO_TITLE) == 0) {
		int minw = 0, minh = 0;
		x_get_min_size(x, win, &minw, &minh);
		if(win->xinfo->wsr.w < minw)
			win->xinfo->wsr.w = minw;
		if(win->xinfo->wsr.h < minh)
			win->xinfo->wsr.h = minh;

		int32_t maxh = x->displays[win->xinfo->display_index].g->h - x->config.xwm_theme.titleH;
		if(win->xinfo->wsr.h > maxh)
			win->xinfo->wsr.h = maxh;
	}

	if(win->xinfo->state == XWIN_STATE_MAX) {
		win->xinfo->wsr.x = 0;
		win->xinfo->wsr.w = x->displays[win->xinfo->display_index].g->w;

      	if((win->xinfo->style & XWIN_STYLE_NO_TITLE) == 0) {
			win->xinfo->wsr.y = x->config.xwm_theme.titleH;
			win->xinfo->wsr.h = x->displays[win->xinfo->display_index].g->h - x->config.xwm_theme.titleH;
		}
		else {
			win->xinfo->wsr.y = 0;
			win->xinfo->wsr.h = x->displays[win->xinfo->display_index].g->h;
		}
	}

	if(wsr_w != win->xinfo->wsr.w || wsr_h != win->xinfo->wsr.h) {
		type = type | X_UPDATE_REBUILD | X_UPDATE_REFRESH;
	}

	if(get_xwm_win_space(x, (int)win->xinfo->style, (int)win->xinfo->state,
			&win->xinfo->wsr,
			&win->xinfo->winr) != 0)	
		return -1;

	/* frame_g is sized from winr, not wsr. Theme/style/title changes can
	 * change the outer frame size even when the workspace size stays the
	 * same, so force a rebuild whenever winr geometry changes. */
	if(winr_w != win->xinfo->winr.w || winr_h != win->xinfo->winr.h) {
		type = type | X_UPDATE_REBUILD | X_UPDATE_REFRESH;
	}
	
	if((type & X_UPDATE_REBUILD) != 0 ||
			win->ws_g_shm == NULL ||
			win->frame_g_shm == NULL ||
			win->ws_g == NULL) {

		release_graph_shm(&win->ws_g, &win->ws_g_shm, &win->xinfo->ws_g_shm_id);

		if(win->ws_g_buffer != NULL) {
			graph_free(win->ws_g_buffer);
			win->ws_g_buffer = NULL;
		}

		release_graph_shm(&win->frame_g, &win->frame_g_shm, &win->xinfo->frame_g_shm_id);

		uint32_t uuid = proc_get_uuid(from_pid);
		key_t key = 0;
		int32_t ws_g_shm_id = xserver_alloc_shm(uuid ^ 0x57530000u,
						win->xinfo->wsr.w * win->xinfo->wsr.h * 4,
						0666|IPC_CREAT|IPC_EXCL, &key);
		if(ws_g_shm_id == -1)
			return -1;

		win->ws_g_shm = shmat(ws_g_shm_id, 0, 0);
		if(win->ws_g_shm == (void*)-1) {
			win->ws_g_shm = NULL;
			return -1;
		}

		win->xinfo->ws_g_shm_id = ws_g_shm_id;
		win->ws_g = graph_new(win->ws_g_shm, win->xinfo->wsr.w, win->xinfo->wsr.h);
		graph_clear(win->ws_g, 0x0);
		win->ws_g_buffer = graph_new(NULL, win->xinfo->wsr.w, win->xinfo->wsr.h);
		graph_clear(win->ws_g_buffer, 0x0);

		int32_t frame_g_shm_id = xserver_alloc_shm(uuid ^ 0x46520000u,
						win->xinfo->winr.w * win->xinfo->winr.h * 4,
						0666|IPC_CREAT|IPC_EXCL, &key);
		if(frame_g_shm_id == -1) {
			release_graph_shm(&win->ws_g, &win->ws_g_shm, &win->xinfo->ws_g_shm_id);
			if(win->ws_g_buffer != NULL) {
				graph_free(win->ws_g_buffer);
				win->ws_g_buffer = NULL;
			}
			return -1;
		}

		win->frame_g_shm = shmat(frame_g_shm_id, 0, 0);
		if(win->frame_g_shm == (void*)-1) {
			win->frame_g_shm = NULL;
			release_graph_shm(&win->ws_g, &win->ws_g_shm, &win->xinfo->ws_g_shm_id);
			if(win->ws_g_buffer != NULL) {
				graph_free(win->ws_g_buffer);
				win->ws_g_buffer = NULL;
			}
			return -1;
		}

		win->xinfo->frame_g_shm_id = frame_g_shm_id;
		win->frame_g = graph_new(win->frame_g_shm, win->xinfo->winr.w, win->xinfo->winr.h);
		win->frame_dirty = true;
		win->ready = false;
		win->has_damage = false;
		win->damage_skip = 0;
	}
	x_update_frame_areas(x, win);
	if((type & X_UPDATE_REFRESH) != 0 || win->xinfo->alpha) {
		mark_all_frame_dirty(x, win->xinfo->display_index);
	}
	/*a hidden window's shadow gets painted over by whatever moves in below
	  it, so it has to be blended fresh when the window shows again*/
	if(!win->xinfo->visible)
		win->shadow_valid = false;
	return 0;
}

static int x_win_space(x_t* x, proto_t* in, proto_t* out) {
	grect_t r;
	int style = proto_read_int(in);
	int state = proto_read_int(in);
	/*a query out of range is treated as a normal window, the same way an
	  unknown state would be*/
	if(state < XWIN_STATE_NORMAL || state > XWIN_STATE_FULL_SCREEN)
		state = XWIN_STATE_NORMAL;
	proto_read_to(in, &r, sizeof(grect_t));
	get_xwm_win_space(x, style, state, &r, &r); 
	PF->add(out, &r, sizeof(grect_t));
	return 0;
}

static int x_repaint_all_win(x_t* x) {
	xevent_t ev;
	ev.type = XEVT_WIN;
	ev.value.window.event = XEVT_WIN_REPAINT;

	xwin_t* win = x->win_tail;
	while(win != NULL) {
		if(win->xinfo != NULL) 
			win->xinfo->update_theme = true;
		x_push_event(x, win, &ev);
		win = win->prev;
	}
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
	x_repaint_all_win(x);
	return 0;
}

static int xwm_theme_update(x_t* x) {
	if(!check_xwm(x))
		return 0;

	proto_t in;
	PF->init(&in)->add(&in, &x->config.xwm_theme, sizeof(xwm_theme_t));
	int res = ipc_call(x->xwm_pid, XWM_CNTL_SET_THEME, &in, NULL);
	PF->clear(&in);
	x_dirty(x, -1);
	return res;
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
		if(win->xinfo != NULL && win->xinfo->display_index == disp_index) {
			x_push_event(x, win, &ev);
		}
		win = win->next;
	}
	return 0;
}

static int xwin_call_xim(x_t* x, proto_t* in, proto_t* out) {
	if(x->im_state.win_xim == NULL) {
		PF->clear(out)->addi(out, -1);
		return -1;
	}

	if(proto_read_int(in) == 0)
		hide_win(x, x->im_state.win_xim);
	else
		show_win(x, x->im_state.win_xim);
	PF->clear(out)->addi(out, 0);
	return 0;
}

static int xserver_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
		int cmd, proto_t* in, proto_t* out, void* p) {
	(void)dev;
	(void)info;
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
		res = xwin_call_xim(x, in, out);
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

static int xserver_win_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int oflag, void* p) {
	(void)dev;
	(void)oflag;
	(void)info;
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
 
static void xwin_launcher(x_t* x, xwin_t* win) {
	if(win == NULL)
		return;

	if(x->win_focus != x->win_launcher) {
		x->win_last = x->win_focus;
		xwin_top(x, x->win_launcher);
	}
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
		else if(pos == FRAME_R_MAX || pos == FRAME_R_CLOSE) {
			return;
		}
		else if(win->xinfo->style & XWIN_STYLE_NO_FRAME) {
			x->current.old_pos.x = x->cursor.cpos.x;
			x->current.old_pos.y = x->cursor.cpos.y;
			x->current.win_drag = win;
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
		if(pos == FRAME_R_RESIZE) {//window resize
			return;
		}

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
					/*graph_free(win->ws_g);
					shmdt(win->ws_g_shm);
					win->ws_g = NULL;
					win->ws_g_shm = NULL;

					if(win->ws_g_buffer != NULL) {
						graph_free(win->ws_g_buffer);
						win->ws_g_buffer = NULL;
					}
					*/
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

	if(ev->type == XEVT_WIN && ev->value.window.event == XEVT_WIN_NONE)
		return;
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
	if(x->mouse_state.state == MOUSE_STATE_NONE && ev->state == MOUSE_STATE_UP)
		return 0;

	if(ev->value.mouse.relative != 0) {
		mouse_cxy(x, x->current_display, ev->value.mouse.rx, ev->value.mouse.ry);
		ev->value.mouse.x = x->cursor.cpos.x;
		ev->value.mouse.y = x->cursor.cpos.y;
	}
	else {
		x->cursor.cpos.x = ev->value.mouse.x;
		x->cursor.cpos.y = ev->value.mouse.y;
	}

	ev->value.mouse.rx = ev->value.mouse.x - x->mouse_state.last_pos.x;
	ev->value.mouse.ry = ev->value.mouse.y - x->mouse_state.last_pos.y;
	x->mouse_state.last_pos.x = ev->value.mouse.x;
	x->mouse_state.last_pos.y = ev->value.mouse.y;

	x_display_t *display = &x->displays[x->current_display];
	display->cursor_task = true;
	cursor_safe(x, display);
	if(ev->state ==  MOUSE_STATE_DOWN) {
		x->cursor.down = true;
		if(x->mouse_state.state == 0) {
			x->mouse_state.state = MOUSE_STATE_DOWN;
			x->mouse_state.down_pos.x = ev->value.mouse.x;
			x->mouse_state.down_pos.y = ev->value.mouse.y;
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
		}
	}
	else if(ev->state ==  MOUSE_STATE_UP) {
		x->cursor.down = false;
		x->mouse_state.state = MOUSE_STATE_NONE;
		ev->value.mouse.from_x = x->mouse_state.down_pos.x;
		ev->value.mouse.from_y = x->mouse_state.down_pos.y;
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

static int xserver_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)dev;
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
	else if(cmd == X_DCNTL_GET_EVT_NODE) {
		x_get_event_node(from_pid, ret);
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
	else if(cmd == X_DCNTL_SHOW_CURSOR) {
		x->show_cursor = (bool)proto_read_int(in);
		x_dirty(x, -1);
	}
	else if(cmd == X_DCNTL_NEXT_FOCUS) {
		xwin_t* win = get_next_focus_win(x, true);
		if(win != NULL)
			xwin_top(x, win);
	}
	else if(cmd == X_DCNTL_CLOSE_FOCUS) {
		xwin_close(x, x->win_focus);
	}
	else if(cmd == X_DCNTL_LAUNCHER) {
		xwin_launcher(x, x->win_focus);
	}
	else if(cmd == X_DCNTL_QUIT) {
		x_quit(from_pid);
	}
	return 0;
}

static int xserver_win_close(vdevice_t* dev, int fd, int from_pid, uint32_t node, fsinfo_t* fsinfo, void* p) {
	(void)dev;
	(void)fsinfo;
	x_t* x = (x_t*)p;
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL) {
		return -1;
	}

	if(win->busy && get_mouse_owner(x, NULL) == win)
		x_cursor_set_busy(x, false);

	int disp_index = win->xinfo != NULL ? win->xinfo->display_index : -1;
	int main_pid = win->from_main_pid;
	x_del_win(x, win);	
	if(!has_win_by_main_pid(x, main_pid))
		x_quit(main_pid);

	x_dirty(x, disp_index);
	return 0;
}

static int _disp_index = -1;
int xserver_step(vdevice_t* dev, void* p) {
	(void)dev;
	x_t* x = (x_t*)p;

	uint64_t tik = kernel_tic_ms(0);
	uint32_t tm = 1000/x->config.fps;

	ipc_disable();
	check_wins(x);
	for(uint32_t i=0; i<x->display_num; i++) {
		x_repaint(x, i);
	}
	ipc_enable();

	/*the flush is a plain outbound IPC to the fb daemon and touches no
	  window state, so it runs with inbound IPC re-enabled. It waits for the
	  daemon to finish copying: compositing now writes straight into the
	  scan-out dma, so the next frame must not overwrite it while the daemon
	  is still pushing it to the panel (tearing/flicker, seen on real panels
	  like raspix whose framebuffer is scanned out continuously).*/
	for(uint32_t i=0; i<x->display_num; i++) {
		if(x->displays[i].pending_flush) {
			fb_flush(&x->displays[i].fb, true);
			x->displays[i].pending_flush = false;
		}
	}

	uint32_t gap = (uint32_t)(kernel_tic_ms(0) - tik);
	if(gap < tm) {
		gap = tm - gap;
		proc_usleep(gap*1000);
	}
	return 0;
}

char* xserver_dev_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p);

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

	x_t x;
	if(x_init(&x, display_man, _disp_index) != 0)
		return -1;

	read_config(&x, "/etc/x/x.json");
	cursor_init("default", &x.cursor);
	x_load_theme("default", &x.config.theme);
	x_dirty(&x, -1);

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
	x.dev = &dev;

	device_run(&dev, mnt_point, FS_TYPE_CHAR | FS_TYPE_ANNOUNIMOUS, 0666);
	x_close(&x);
	return 0;
}
