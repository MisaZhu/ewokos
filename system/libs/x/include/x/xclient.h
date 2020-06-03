#ifndef XCLIENT_H
#define XCLIENT_H

#include <graph/graph.h>
#include <x/xcntl.h>
#include <x/xevent.h>

typedef struct st_xevent {
  xevent_t event;
  struct st_xevent* next;
} x_event_t;

typedef struct st_x {
	int fd;
	void* data;
	xinfo_t xinfo_prev; //for backup the state before fullscreen/min/max.
	bool closed;
	bool repaint;

	x_event_t* event_head;
  x_event_t* event_tail;

	void (*on_init)(struct st_x* x);
	void (*on_close)(struct st_x* x);
	void (*on_min)(struct st_x* x);
	void (*on_resize)(struct st_x* x);
	void (*on_focus)(struct st_x* x);
	void (*on_unfocus)(struct st_x* x);
	void (*on_repaint)(struct st_x* x, graph_t* g);
	void (*on_loop)(struct st_x* x);
	void (*on_event)(struct st_x* x, xevent_t* ev);
} x_t;

x_t*     x_open(int x, int y, int w, int h, const char* title, int style);
int      x_update_info(x_t* x, xinfo_t* xinfo);
int      x_get_info(x_t* x, xinfo_t* xinfo);
void     x_close(x_t* x);
int      x_screen_info(xscreen_t* scr);
int      x_is_top(x_t* x);
int      x_set_visible(x_t* x, bool visible);
void     x_repaint(x_t* x);
void     x_run(x_t* x);

#endif
