#ifndef XCLIENT_H
#define XCLIENT_H

#include <graph/graph.h>
#include <x/xcntl.h>
#include <x/xevent.h>

typedef struct st_x {
	int fd;
	xinfo_t xinfo_prev; //for backup the state before fullscreen/min/max.
	int closed;

	void* data;
	void (*on_max)(struct st_x* x, void* p);
	void (*on_close)(struct st_x* x, void* p);
	void (*on_min)(struct st_x* x, void* p);
	void (*on_restore)(struct st_x* x, void* p);
	void (*on_resize)(struct st_x* x, void* p);
	void (*on_focus)(struct st_x* x, void* p);
	void (*on_unfocus)(struct st_x* x, void* p);
} x_t;

x_t*     x_open(int x, int y, int w, int h, const char* title, int style);
graph_t* x_get_graph(x_t* x);
void     x_release_graph(x_t* x, graph_t* g);
int      x_update(x_t* x);
int      x_update_info(x_t* x, xinfo_t* xinfo);
int      x_get_info(x_t* x, xinfo_t* xinfo);
void     x_close(x_t* x);
int      x_get_event(x_t* x, xevent_t* ev);
int      x_screen_info(xscreen_t* scr);
int      x_is_top(x_t* x);
int      x_set_visible(x_t* x, bool visible);

#endif
