#ifndef XWM_H
#define XWM_H

#include <graph/graph.h>
#include <x/xcntl.h>

enum {
	XWM_FRAME_TITLE = 0,
	XWM_FRAME_CLOSE,
	XWM_FRAME_MAX,
	XWM_FRAME_MIN
};

enum {
	XWM_CNTL_DRAW_FRAME = 0,
	XWM_CNTL_DRAW_DESKTOP,
	XWM_CNTL_GET_WORKSPACE,
	XWM_CNTL_GET_POS
};

typedef struct {
	void* data;
	void (*draw_desktop)(graph_t* g, void* p);
	void (*draw_frame)(graph_t* g, xinfo_t* info, bool top, void* p);
	int  (*get_pos)(int x, int y, xinfo_t* info, void* p);
	void (*get_workspace)(int style, grect_t* xr, grect_t* wsr, void* p);
} xwm_t;

void xwm_run(xwm_t* xwm);

#endif
