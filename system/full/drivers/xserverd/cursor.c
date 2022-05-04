#include "cursor.h"

static inline void draw_cursor_mouse(graph_t* g, int mx, int my, int mw, int mh) {
	graph_line(g, mx+1, my, mx+mw-1, my+mh-2, 0xffffffff);
	graph_line(g, mx, my, mx+mw-1, my+mh-1, 0xff000000);
	graph_line(g, mx, my+1, mx+mw-2, my+mh-1, 0xffffffff);

	graph_line(g, mx, my+mh-2, mx+mw-2, my, 0xffffffff);
	graph_line(g, mx, my+mh-1, mx+mw-1, my, 0xff000000);
	graph_line(g, mx+1, my+mh-1, mx+mw-1, my+1, 0xffffffff);
}

static inline void draw_cursor_touch(graph_t* g, int mx, int my, int mw, int mh) {
	int r = mw/2 <= mh/2 ? mw/2 : mh/2;
	graph_fill_circle(g, mx+r, my+r, r, 0x88ffffff);
	graph_fill_circle(g, mx+r, my+r, r*2/3, 0x88000000);
}

void draw_cursor(graph_t* g, int mx, int my, int mw, int mh, int type) {
	if(type == CURSOR_TOUCH)
		draw_cursor_touch(g, mx, my, mw, mh);
	else
		draw_cursor_mouse(g, mx, my, mw, mh);
}