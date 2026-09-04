#ifndef XSERVER_H
#define XSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <display/display.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xtheme.h>
#include <x/xwm.h>
#include <ewoksys/vdevice.h>
#include <displayman/displayman.h>
#include "cursor.h"
#include "xevtpool.h"

#define X_EVENT_MAX 16

/* a window stuck !ready (client stalled behind the input IPC storm after
   a resize/rebuild, or an fps_async present the client dropped because the
   server still owned the handoff buffer) must not throttle the whole display
   forever: once it has been stuck this long the repaint proceeds without it.
   The composite loop skips !ready windows, so its area simply shows what is
   below until the client catches up with its next UPDATE. The same budget
   paces x_poll_updates asking such a window for its picture again. */
#define X_NOT_READY_TIMEOUT_MS 500

enum {
	X_win_DRAG_MOVE = 1,
	X_win_DRAG_RESIZE
};

typedef struct st_xwin {
	int fd;
	int from_pid; 
	int from_main_pid; //main proc pid
	uint32_t from_main_pid_uuid;

	graph_t* ws_g; //workspace graph, owns its shm canvas (graph_new_shm)
	graph_t* ws_g2; //second workspace graph for fps_async double-buffering (graph_new_shm), NULL when fps_async=0
	graph_t* frame_g; //frame graph, owns its shm canvas (graph_new_shm)

	xinfo_t* xinfo;
	bool dirty;
	bool ready;
	bool frame_dirty;
	/*the shadow bands of this window already sit blended on the display:
	  blending them again would darken them further, so they only get
	  blended again after what is below them was repainted*/
	bool shadow_valid;
	grect_t shadow_rect; //winr the shadow bands were blended for
	bool dirty_mark;
	bool busy;

	/*tic (ms) when this window was last seen visible and !ready; 0 while
	  ready. all_win_ready() stops throttling repaints for the whole
	  display once a window has been stuck past X_NOT_READY_TIMEOUT_MS.*/
	uint64_t not_ready_ms;

	/*tic (ms) since the compositor first found xinfo->painting set; 0 while
	  the canvas is readable. A client that goes idle after its last frame
	  never clears painting, so win_src_stable() stops waiting once this
	  passes X_PAINT_TIMEOUT_MS.*/
	uint64_t paint_ms;

	/*tic (ms) since a frame was accepted but not yet composited; 0 when no
	  accept is outstanding. The client is parked (blocking) or locked out of
	  its handoff buffer (fps_async) for that whole time, so x_poll_updates
	  releases it once this passes X_ACCEPT_TIMEOUT_MS.*/
	uint64_t accept_ms;

	/*tic (ms) of the last XEVT_WIN_REPAINT sent to a window that never became
	  ready; 0 while none is outstanding. Bounds how often x_poll_updates asks
	  such a window for its picture again.*/
	uint64_t repaint_req_ms;

	/*the hit-test areas below describe the CURRENT winr. Cleared whenever xwm
	  refused GET_FRAME_AREAS: they are absolute screen rects derived from
	  winr, so keeping the previous set would leave the close/min/max/resize
	  tests describing the old, smaller frame - they land inside the workspace
	  after a resize. xwin_revalidate_geometry retries until this is true.*/
	bool frame_areas_valid;

	grect_t r_title;
	grect_t r_close;
	grect_t r_min;
	grect_t r_max;
	grect_t r_resize;

	struct st_xwin *next;
	struct st_xwin *prev;
} xwin_t;

typedef struct {
	xwin_t* win_drag; //moving or resizing;
	gpos_t old_pos;
	gpos_t pos_delta;
	uint32_t drag_state;
	/*drag frame overlay: the scene pixels under the last-drawn outline,
	  so moving the outline restores the scene from the band instead of
	  repainting the whole display for every drag step*/
	graph_t* drag_band;
	grect_t drag_rect;      //where drag_band was taken from (clipped)
	int32_t drag_display;   //display the band belongs to
	bool drag_band_valid;   //the outline is on screen at drag_rect
} x_current_t;

typedef struct {
	uint32_t fps;
	bool force_fullscreen;
	uint32_t bg_proc_priority;

	graph_t* logo;
	x_theme_t theme;
	xwm_theme_t xwm_theme;

	bool multi_task;
	bool fps_async;   /*0: client repaint blocks until the server snapshots it (client fps capped to server fps).
	                    1: double-buffered, client repaint never blocks and keeps its own fps.*/
} x_conf_t;

typedef struct {
	uint32_t display_index;
	display_t display;
	graph_t* g;
	int32_t  g_shm_id;
	graph_t* g_display;
	grect_t desktop_rect;
	bool dirty;
	bool cursor_task;
	bool need_repaint;
	bool pending_flush; //flush is issued outside the server lock section
	/*full rebuilds spent waiting for a window's painter to finish: a rebuild
	  paints the desktop over everything, so a mid-frame window cannot simply
	  be skipped the way an incremental repaint skips it*/
	uint32_t paint_wait;
	/*a frame owns the ctrl dirty list and the scan-out buffer from its
	  display_set_dirty() until its flush lands: the input handler's
	  fast cursor redraw must stay out for that whole window, or it
	  would overwrite the frame's dirty rects before the daemon reads
	  them*/
	bool flush_inflight;
	uint32_t wait_ready; //frames spent waiting for the windows to get ready

	bool active;
} x_display_t;

typedef struct {
	gpos_t down_pos;
	gpos_t last_pos;
	uint32_t state; 
	bool busy;
} x_mouse_state_t;

typedef struct {
	xwin_t* win_xim;
	bool    win_xim_actived;
	int     down_win_fd;
} x_im_state_t;

typedef struct {
	const char* display_man;
	x_display_t displays[DISP_MAX];
	uint32_t display_num;
	uint32_t current_display;

	int xwm_pid;
	uint32_t xwm_uuid;
	bool xwm_changed; //a new xwm registered: window geometry must be revalidated

	bool show_cursor;
	cursor_t cursor;

	xwin_t* win_head;
	xwin_t* win_tail;
	xwin_t* win_focus;
	xwin_t* win_launcher;
	xwin_t* win_last;

	x_mouse_state_t mouse_state;
	x_im_state_t im_state;
	x_current_t current;
	x_conf_t config;
	vdevice_t* dev;
} x_t;

/*the canvas the compositor reads a window's workspace from. There is no
  private snapshot any more: the buffer the client published into IS the
  snapshot, and it stays put until x_update_commit hands it back. In
  fps_async mode that is the handoff buffer the client copied its frame onto
  (front_index, published before the barrier), otherwise the client's own
  render target - which win_src_stable() only lets the compositor touch while
  the painter is parked or idle.*/
static inline graph_t* win_comp_src(xwin_t* win) {
	if(win->xinfo != NULL && win->xinfo->fps_async &&
			win->xinfo->front_index == 1)
		return win->ws_g2;
	return win->ws_g;
}

/*maximized and fullscreen windows fill the display edge to edge: xwm draws
  no frame, shadow or rounded corners for them, so no translucent frame
  pixels cut into their workspace*/
static inline bool win_edge_to_edge(xwin_t* win) {
	return win->xinfo->state == XWIN_STATE_MAX ||
			win->xinfo->state == XWIN_STATE_FULL_SCREEN;
}

/*theme alpha here only means the frame has translucent edge/corner pixels;
  the workspace interior still gets copied opaquely and should not force the
  whole window into the expensive translucent-window path.

  Such frames are blended on top of the workspace (win_dirty forces
  frame_dirty for them), so their ws part has to live inside frame_g:
  xserverd copies it there and xwm draws the decorations over it.*/
static inline bool frame_cuts_ws(x_t* x, xwin_t* win) {
	if((win->xinfo->style & XWIN_STYLE_NO_FRAME) != 0)
		return false;
	if(win_edge_to_edge(win))
		return false;
	return x->config.xwm_theme.frameAlpha;
}

/*the theme's background effect blends the desktop over the WHOLE
  window of an unfocused one (xwm's drawBGEffect runs over the entire
  winr). While that blend is live the workspace pixels only exist in
  frame_g, so content changes have to be copied there. Every other
  window without a translucent frame keeps pristine workspace pixels in
  the snapshot and gets composited straight from it.*/
static inline bool win_bg_effect_active(x_t* x, xwin_t* win) {
	return x->config.xwm_theme.bgEffect != 0 &&
			!win->xinfo->focused &&
			(win->xinfo->style & XWIN_STYLE_NO_BG_EFFECT) == 0;
}

/*small rectangle helpers shared between the compositor modules*/
static inline bool rect_is_valid(const grect_t* r) {
	return r->w > 0 && r->h > 0;
}

static inline bool rect_contains(const grect_t* out, const grect_t* in) {
	return in->x >= out->x && in->y >= out->y &&
			(in->x + in->w) <= (out->x + out->w) &&
			(in->y + in->h) <= (out->y + out->h);
}

static inline void rect_union_to(grect_t* dst, const grect_t* src) {
	int32_t x0 = dst->x < src->x ? dst->x : src->x;
	int32_t y0 = dst->y < src->y ? dst->y : src->y;
	int32_t x1 = (dst->x + dst->w) > (src->x + src->w) ? (dst->x + dst->w) : (src->x + src->w);
	int32_t y1 = (dst->y + dst->h) > (src->y + src->h) ? (dst->y + dst->h) : (src->y + src->h);
	dst->x = x0;
	dst->y = y0;
	dst->w = x1 - x0;
	dst->h = y1 - y0;
}

static inline bool rect_overlap_or_touch(const grect_t* a, const grect_t* b) {
	if((a->x + a->w) < b->x || (b->x + b->w) < a->x)
		return false;
	if((a->y + a->h) < b->y || (b->y + b->h) < a->y)
		return false;
	return true;
}

static inline bool rect_clip_to_graph(graph_t* g, grect_t* r) {
	grect_t bounds = {0, 0, g->w, g->h};
	return grect_insect(&bounds, r);
}

/*core state helpers living in xserverd.c*/
bool check_xwm(x_t* x);
void x_dirty(x_t* x, int32_t display_index);
void x_repaint_req(x_t* x, int32_t display_index);

/*server-wide state lock (xserver_dev.c): IPC_MULTI_TASK dispatches the
  handlers on concurrent kernel worker threads, so every access to x_t,
  the window list and the event pools must hold this mutex. Handlers may
  block in outbound IPC (xwm geometry, vfs_wakeup) while holding it; the
  blocking fb flush and the step pacing sleep deliberately run outside.*/
extern pthread_mutex_t x_server_lock;
void x_server_lock_enter(void);
void x_server_lock_leave(void);

/*vdevice entry points living in xserver_dev.c*/
int xserver_step(vdevice_t* dev, void* p);
int xserver_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
		int cmd, proto_t* in, proto_t* out, void* p);
int xserver_win_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int oflag, void* p);
int xserver_win_close(vdevice_t* dev, int fd, int from_pid, uint32_t node, fsinfo_t* fsinfo, void* p);
int xserver_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p);

#endif
