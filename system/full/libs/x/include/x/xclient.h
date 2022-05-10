#ifndef XCLIENT_H
#define XCLIENT_H

#include <graph/graph.h>
#include <fonts/fonts.h>
#include <x/xcntl.h>
#include <x/xevent.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct st_xevent {
  xevent_t event;
  struct st_xevent* next;
} x_event_t;

struct st_xwin;

typedef struct st_x {
	struct st_xwin* main_win;
	void* data;
	bool terminated;
	x_event_t* event_head;
	x_event_t* event_tail;

	void (*on_loop)(void* p);
} x_t;

typedef struct st_xwin {
	x_t* x;
	int fd;
	void* data;
	void* g_buf;
	int   g_shmid;
	xinfo_t xinfo_prev; //for backup the state before fullscreen/min/max.

	void (*on_close)(struct st_xwin* xwin);
	void (*on_min)(struct st_xwin* xwin);
	void (*on_resize)(struct st_xwin* xwin);
	void (*on_focus)(struct st_xwin* xwin);
	void (*on_unfocus)(struct st_xwin* xwin);
	void (*on_repaint)(struct st_xwin* xwin, graph_t* g);
	void (*on_event)(struct st_xwin* xwin, xevent_t* ev);
} xwin_t;

xwin_t*  x_open(x_t* xp, int x, int y, int w, int h, const char* title, int style);
int      x_update_info(xwin_t* x, const xinfo_t* xinfo);
int      x_get_info(xwin_t* x, xinfo_t* xinfo);
void     x_close(xwin_t* x);
int      x_screen_info(xscreen_t* scr);
int      x_set_visible(xwin_t* x, bool visible);
void     x_repaint(xwin_t* x, bool thread);
int      x_resize(xwin_t* x, int dw, int dh);
int      x_resize_to(xwin_t* x, int w, int h);
int      x_move(xwin_t* xwin, int dx, int dy);
int      x_move_to(xwin_t* xwin, int x, int y);
int      x_call_xim(xwin_t* xwin);
int      x_set_display(xwin_t* xwin, uint32_t display_index);

void     x_init(x_t* x, void* data);
void     x_run(x_t* x, void* loop_data);

#ifdef __cplusplus
}
#endif

#endif
