#include "MacWM.h"
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <upng/upng.h>
#include <x++/X.h>
#include <stdlib.h>
#include <string.h>

using namespace Ewok;

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
	font_text_size(info->title, font, fontSize, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	
	/*grect_t rect;
	getTitle(info, &rect);
	*/

	int pw = (r->w-sz.w)/2;
	int ph = (r->h-sz.h)/2;
	graph_fill(g, r->x, r->y, r->w, r->h, bg);//title box
	if(top)
		drawTitlePattern(g, r->x, r->y, r->w, r->h, fg);

	graph_fill(g, r->x+pw-2, r->y, sz.w+4, r->h, bg);//title box
	graph_draw_text_font(g, r->x+pw, r->y+ph, info->title, font, fontSize, fg);//title
	graph_line(g, r->x, r->y+r->h-1, r->x+r->w, r->y+r->h-1, fg);//title box
}

/*void MacWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
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
*/

void MacWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	if(!top)
		return;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	graph_line(g, 
			r->x + r->w - frameW + 1, r->y,
			r->x + r->w, r->y, dark);
	graph_line(g,
			r->x + r->w - frameW + 1, r->y + 1,
			r->x + r->w, r->y + 1, bright);
	graph_line(g,
			r->x, r->y + r->h - frameW + 1,
			r->x, r->y + r->h, dark);
	graph_line(g,
			r->x + 1, r->y + r->h - frameW + 1,
			r->x + 1, r->y + r->h, bright);
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
}

MacWM::MacWM(void) {
	desktopBGColor = 0xff555588;
	desktopFGColor = 0xff8888aa;
	bgColor = 0xff222222;
	fgColor = 0xff888888;
	bgTopColor = 0xffaaaaaa;
	fgTopColor = 0xff222222;
	titleH = 32;
}