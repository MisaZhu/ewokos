#include "MacWM.h"
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <upng/upng.h>
#include <x/x.h>
#include <sconf/sconf.h>
#include <stdlib.h>

using namespace Ewok;

void MacWM::loadConfig(sconf_t* sconf) {
	XWM::loadConfig(sconf);
	const char* v = sconf_get(sconf, "pattern");
	pattern = png_image_new_bg(x_get_theme_fname(X_THEME_ROOT, "xwm", v), desktopBGColor);
}

graph_t* MacWM::genPattern(void) {
	int sz = 1; 
	int x = 0;
	int y = 0;
	bool shift = false;
	graph_t* g = graph_new(NULL, 64, 64);
	graph_clear(g, desktopBGColor);

	if(sz <= 1)
		sz = 1;

	if(sz > 1) {
		for(int i=0; y<g->h; i++) {
			for(int j=0; x<g->w;j++) {
				graph_fill(g, x, y, sz, sz, desktopFGColor);
				x += sz*2;
			}
			x = shift ? 0:sz;
			shift = !shift;
			y += sz;
		}
	}
	else {
		for(int i=0; y<g->h; i++) {
			for(int j=0; x<g->w;j++) {
				graph_pixel(g, x, y, desktopFGColor);
				x += 2;
			}
			x = shift ? 0:sz;
			shift = !shift;
			y += sz;
		}
	}
	return g;
}

void MacWM::drawDesktop(graph_t* g) {
	if(pattern == NULL)
		pattern = genPattern();

	graph_clear(g, 0xffffffff);
	int x = 0;
	int y = 0;
	for(int i=0; y<g->h; i++) {
		for(int j=0; x<g->w;j++) {
			graph_blt(pattern, 0, 0, pattern->w, pattern->h,
					g, x, y, pattern->w, pattern->h);
			x += pattern->w;
		}
		x = 0;
		y += pattern->h;
	}
}

void MacWM::drawTitlePattern(graph_t* g, int x, int y, int w, int h, uint32_t fg) {
	int step = 3;
	y = y + step;
	int steps = h / step;

	for (int i = 1; i < steps-1; i++) {
		graph_line(g, x + 4, y, x + w - 8, y, fg);
		y += step;
	}
}

void MacWM::drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	gsize_t sz;
	font_text_size(info->title, &font, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	
	grect_t rect;
	getTitle(info, &rect);

	int pw = (r->w-sz.w)/2;
	int ph = (r->h-sz.h)/2;
	graph_fill(g, r->x, r->y, r->w, rect.h, bg);//title box
	if(top) {
		drawTitlePattern(g, r->x, r->y, pw, r->h, fg);
		drawTitlePattern(g, r->x+pw+sz.w, r->y, pw, r->h, fg);
	}
	graph_draw_text_font(g, r->x+pw, r->y+ph, info->title, &font, fg);//title
}


MacWM::~MacWM(void) {
	if(pattern != NULL)
		graph_free(pattern);
	font_close(&font);
}

MacWM::MacWM(void) {
	desktopBGColor = 0xff555588;
	desktopFGColor = 0xff8888aa;
	bgColor = 0xff222222;
	fgColor = 0xff888888;
	bgTopColor = 0xffaaaaaa;
	fgTopColor = 0xff222222;
	titleH = 32;
	pattern = NULL;
}