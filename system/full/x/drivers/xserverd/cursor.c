#include "cursor.h"
#include <string.h>
#include <upng/upng.h>

static inline void draw_cursor_raw(graph_t* g, int mx, int my, int mw, int mh, bool down) {
	(void)down;
	int r = mw/2 <= mh/2 ? mw/2 : mh/2;
	graph_fill_circle(g, mx+r, my+r, r, 0x88ffffff);
	graph_fill_circle(g, mx+r, my+r, r*2/3, 0x88000000);
}

void draw_cursor(graph_t* g, cursor_t* cursor, int mx, int my) {
	if(cursor->img == NULL) {	
		draw_cursor_raw(g, mx, my, cursor->size.w, cursor->size.h, cursor->down);
		return;
	}

	graph_blt_alpha(cursor->img, 0, 0, cursor->img->w, cursor->img->h,
			g, mx, my, cursor->img->w, cursor->img->h, 0xff);
}

void cursor_init(const char* theme, cursor_t* cursor) {
	char fname[256];
	if(cursor->type == CURSOR_TOUCH) {
		if(theme == NULL || theme[0] == 0)
			theme = "default";
		snprintf(fname, 255, "/user/x/themes/%s/xwm/cursors/touch.png", theme);
	}
	else if(cursor->type == CURSOR_MOUSE) {
		if(theme == NULL || theme[0] == 0)
			theme = "default";
		snprintf(fname, 255, "/user/x/themes/%s/xwm/cursors/mouse.png", theme);
	}
	else {
		fname[0] == 0;
	}
	klog("[%s]\n", fname);
	if(fname[0] != 0)
		cursor->img = png_image_new(fname);

	if(cursor->img == NULL) {
		cursor->size.w = 21;
		cursor->size.h = 21;
		cursor->offset.x = cursor->size.w/2;
		cursor->offset.y = cursor->size.h/2;
	}
	else {
		cursor->size.w = cursor->img->w; 
		cursor->size.h = cursor->img->h; 
	}
}