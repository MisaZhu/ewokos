#include "MacWM.h"
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <upng/upng.h>
#include <x/x.h>
#include <sconf/sconf.h>
#include <stdlib.h>
#include <string.h>

using namespace Ewok;

void MacWM::loadConfig(sconf_t* sconf) {
	XWM::loadConfig(sconf);
	const char* v = sconf_get(sconf, "pattern");
	if(v[0] != 0 && strcmp(v, "none") != 0)
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
		while(y < g->h) {
			while(x < g->w) {
				graph_fill(g, x, y, sz, sz, desktopFGColor);
				x += sz*2;
			}
			x = shift ? 0:sz;
			shift = !shift;
			y += sz;
		}
	}
	else {
		while(y < g->h) {
			while(x < g->w) {
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
	y = y + step+2;
	int steps = (h-2) / step;

	for (int i = 1; i < steps-1; i++) {
		graph_line(g, x + 2, y, x + w - 4, y, fg);
		y += step;
	}
}

void MacWM::drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	gsize_t sz;
	font_text_size(info->title, &font, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	
	/*grect_t rect;
	getTitle(info, &rect);
	*/

	int pw = (r->w-sz.w)/2;
	int ph = (r->h-sz.h)/2;
	graph_fill(g, r->x, r->y, r->w, r->h, bg);//title box
	if(top) {
		drawTitlePattern(g, r->x, r->y, r->w, r->h, fg);
	}
	graph_fill(g, r->x+pw-2, r->y, sz.w+4, r->h, bg);//title box
	graph_draw_text_font(g, r->x+pw, r->y+ph, info->title, &font, fg);//title
	graph_line(g, r->x, r->y+r->h, r->x+r->w, r->y+r->h, fg);//title box
}

void MacWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	if(!top)
		return;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill(g, r->x, r->y, r->w, r->h, bg & 0x88ffffff);
	graph_fill(g, r->x+3, r->y+3, r->w-6, r->h-6, 0x88222222);
	graph_line(g, r->x+3, r->y+r->h-4, r->x+r->w-4, r->y+3, bg & 0x88ffffff);
	graph_box(g, r->x, r->y, r->w, r->h, fg & 0x88ffffff);
}

void MacWM::getTitle(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x;
	rect->y = info->winr.y;
	rect->w = info->winr.w;
	rect->h = titleH;
}

void MacWM::getClose(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x + 8;
	rect->y = info->winr.y;// - titleH;
	rect->w = titleH;
	rect->h = titleH-1;
}

void MacWM::getMin(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x + info->winr.w - titleH*2 - 8;
	rect->y = info->winr.y;// - titleH;
	rect->w = titleH;
	rect->h = titleH-1;
}

void MacWM::getMax(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x + info->winr.w- titleH - 8;
	rect->y = info->winr.y;// - titleH;
	rect->w = titleH;
	rect->h = titleH-1;
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