#ifndef XWIN_H
#define XWIN_H

#include <graph/graph.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <pthread.h>
#include <x/x.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct st_xwin {
	x_t* x;
	int fd;
	void* data;

	void* ws_g_shm;
	int32_t xinfo_shm_id;
	xinfo_t *xinfo;
	xinfo_t xinfo_prev; //for backup the state before fullscreen/min/max.
	pthread_mutex_t painting_lock;

	bool (*on_close)(struct st_xwin* xwin);
	void (*on_min)(struct st_xwin* xwin);
	void (*on_resize)(struct st_xwin* xwin);
	void (*on_move)(struct st_xwin* xwin);
	void (*on_focus)(struct st_xwin* xwin);
	void (*on_unfocus)(struct st_xwin* xwin);
	void (*on_show)(struct st_xwin* xwin);
	void (*on_hide)(struct st_xwin* xwin);
	void (*on_update_theme)(struct st_xwin* xwin);
	void (*on_repaint)(struct st_xwin* xwin, graph_t* g);
	void (*on_reorg)(struct st_xwin* xwin);
	void (*on_event)(struct st_xwin* xwin, xevent_t* ev);
} xwin_t;

xwin_t*  xwin_open(x_t* xp, int32_t disp_index, int x, int y, int w, int h, const char* title, int style);
void     xwin_close(xwin_t* xwin);
void     xwin_destroy(xwin_t* xwin);
int      xwin_set_visible(xwin_t* xwin, bool visible);
int      xwin_top(xwin_t* xwin);
void     xwin_set_alpha(xwin_t* xwin, bool alpha);
void     xwin_repaint(xwin_t* xwin);
graph_t* xwin_fetch_graph(xwin_t* xwin, graph_t* g);
//void     xwin_repaint_req(xwin_t* xwin);
int      xwin_max(xwin_t* xwin);
int      xwin_fullscreen(xwin_t* xwin);
int      xwin_resize(xwin_t* xwin, int dw, int dh);
int      xwin_resize_to(xwin_t* xwin, int w, int h);
int      xwin_move(xwin_t* xwin, int dx, int dy);
int      xwin_move_to(xwin_t* xwin, int x, int y);
int      xwin_set_display(xwin_t* xwin, uint32_t display_index);
int      xwin_call_xim(xwin_t* xwin, bool show);
int      xwin_event_handle(xwin_t* xwin, xevent_t* ev);
void     xwin_busy(xwin_t* xwin, bool busy);

#ifdef __cplusplus
}
#endif

#endif
