#ifndef XWM_H
#define XWM_H

#include <graph/graph_ex.h>
#include <x/xcntl.h>
#include <x/xtheme.h>

#ifdef __cplusplus
extern "C" {
#endif

#define X_DEFAULT_XWM       "/sbin/x/xwm_opencde"

enum {
	XWM_CNTL_DRAW_FRAME = 0,
	XWM_CNTL_DRAW_DRAG_FRAME,
	XWM_CNTL_DRAW_DESKTOP,
	XWM_CNTL_GET_WIN_SPACE,
	XWM_CNTL_GET_MIN_SIZE,
	XWM_CNTL_GET_FRAME_AREAS,
	XWM_CNTL_SET_THEME
};

typedef struct {
	void* data;
	xwm_theme_t theme;

	void (*get_win_space)(int style, grect_t* xr, grect_t* wsr, void* p);
	void (*get_close)(xinfo_t* info, grect_t* r, void* p);
	void (*get_min)(xinfo_t* info, grect_t* r, void* p);
	void (*get_min_size)(xinfo_t* info, int* w, int* h, void* p);
	void (*get_max)(xinfo_t* info, grect_t* r, void* p);
	void (*get_title)(xinfo_t* info, grect_t* r, void* p);
	void (*get_resize)(xinfo_t* info, grect_t* r, void* p);
	void (*get_frame)(xinfo_t* info, grect_t* r, void* p);

	void (*draw_desktop)(graph_t* g, void* p);
	void (*draw_title)(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p);
	void (*draw_max)(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p);
	void (*draw_min)(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p);
	void (*draw_close)(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p);
	void (*draw_resize)(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p);
	void (*draw_frame)(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top, void* p);
	void (*draw_shadow)(graph_t* desktop_g, graph_t* frame_g, xinfo_t* info, bool top, void* p);
	void (*draw_bg_effect)(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, bool top, void* p);
	void (*draw_drag_frame)(graph_t* g, grect_t* r, void* p);
} xwm_t;

void xwm_run(xwm_t* xwm);

#ifdef __cplusplus
}
#endif

#endif
