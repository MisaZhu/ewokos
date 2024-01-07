#include "cursor.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <x/x.h>

static inline void draw_cursor_raw(graph_t* g, int mx, int my, int mw, int mh, bool down) {
	(void)down;
	int r = mw/2 <= mh/2 ? mw/2 : mh/2;
	graph_fill_circle(g, mx+r, my+r, r, 0x88ffffff);
	graph_fill_circle(g, mx+r, my+r, r*2/3, 0x88000000);
}

void draw_cursor(graph_t* g, cursor_t* cursor, int mx, int my) {
	if(cursor->type == CURSOR_TOUCH && !cursor->down)
		return;

	if(cursor->img == NULL)
		draw_cursor_raw(g, mx, my, cursor->size.w, cursor->size.h, cursor->down);
	else
		graph_blt_alpha(cursor->img, 0, 0, cursor->img->w, cursor->img->h,
				g, mx, my, cursor->img->w, cursor->img->h, 0xff);
}

void cursor_init(const char* theme, cursor_t* cursor) {
	cursor->img = NULL;
	cursor->size.w = 21;
	cursor->size.h = 21;
	cursor->offset.x = cursor->size.w/2;
	cursor->offset.y = cursor->size.h/2;

	char fname[256] = "";
	snprintf(fname, 255, "%s/%s/xwm/cursors/%s.conf", X_THEME_ROOT, theme,
			cursor->type == CURSOR_MOUSE ? "mouse":"touch");
	sconf_t* sconf = sconf_load(fname);
	if(sconf == NULL)
		return;

	const char* v = sconf_get(sconf, "x_offset");
	if(v[0] != 0)
		cursor->offset.x = atoi(v);

	v = sconf_get(sconf, "y_offset");
	if(v[0] != 0)
		cursor->offset.y = atoi(v);
	
	v = sconf_get(sconf, "cursor");
	if(v[0] != 0) {
		if(v[0] != '/')
			snprintf(fname, 255, "%s/%s/xwm/%s", X_THEME_ROOT, theme, v);
		else
			sstrncpy(fname, v, 255);
	}
	cursor->img = png_image_new(fname);

	if(cursor->img != NULL) { 
		cursor->size.w = cursor->img->w; 
		cursor->size.h = cursor->img->h; 
		if(cursor->offset.x >= cursor->size.w)
			cursor->offset.x = cursor->size.w - 1;
		if(cursor->offset.y >= cursor->size.h)
			cursor->offset.y = cursor->size.h - 1;
	}
	sconf_free(sconf);
}