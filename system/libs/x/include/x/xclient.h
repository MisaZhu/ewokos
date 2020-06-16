#ifndef XCLIENT_H
#define XCLIENT_H

#include <graph/graph.h>
#include <x/xcntl.h>
#include <x/xevent.h>

typedef struct st_xevent {
  xevent_t event;
  struct st_xevent* next;
} x_event_t;

typedef struct st_xwin {
	int fd;
	void* data;
	bool terminated;
	xinfo_t xinfo_prev; //for backup the state before fullscreen/min/max.

	void (*on_close)(struct st_xwin* xwin);
	void (*on_min)(struct st_xwin* xwin);
	void (*on_resize)(struct st_xwin* xwin);
	void (*on_focus)(struct st_xwin* xwin);
	void (*on_unfocus)(struct st_xwin* xwin);
	void (*on_repaint)(struct st_xwin* xwin, graph_t* g);
	void (*on_loop)(struct st_xwin* xwin);
	void (*on_event)(struct st_xwin* xwin, xevent_t* ev);

	x_event_t* event_head;
  x_event_t* event_tail;
} xwin_t;

xwin_t*  x_open(int x, int y, int w, int h, const char* title, int style);
int      x_update_info(xwin_t* x, const xinfo_t* xinfo);
int      x_get_info(xwin_t* x, xinfo_t* xinfo);
void     x_close(xwin_t* x);
int      x_screen_info(xscreen_t* scr);
int      x_is_top(xwin_t* x);
int      x_set_visible(xwin_t* x, bool visible);
void     x_repaint(xwin_t* x);
void     x_run(xwin_t* x);

#endif
