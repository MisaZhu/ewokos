#include "EwokWM.h"
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <graph/graph_png.h>
#include <x++/X.h>
#include <stdlib.h>
#include <string.h>

using namespace Ewok;

void EwokWM::drawTitlePattern(graph_t* g, int x, int y, int w, int h, uint32_t fg) {
	int step = 3;
	y = y + step+2;
	int steps = (h-2) / step;

	for (int i = 1; i < steps-1; i++) {
		graph_line(g, x + 2, y, x + w - 4, y, fg);
		y += step;
	}
}

void EwokWM::drawDragFrame(graph_t* g, grect_t* r) {
	int wd = xwm.theme.frameW;
	if(wd <= 0)
		wd = 2;
	graph_frame(g, r->x-wd, r->y-wd, 
			r->w+wd*2, r->h+wd*2, wd, 0x88ffffff, false);
}

void EwokWM:: markFrameRound(graph_t* frame_g, int r) {
	if(r <= 0)
		r = 10;

	graph_t* mask = graph_new(NULL, r, r);
	graph_clear(mask, 0);
	graph_fill_arc(mask, mask->w-1, mask->h-1, r, 90, 180, 0xffffffff);
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, 0, 0, mask->w, mask->h);

	graph_clear(mask, 0);
	graph_fill_arc(mask, mask->w-1, 0, r, 180, 270, 0xffffffff);
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, 0, frame_g->h-mask->h, mask->w, mask->h);

	graph_clear(mask, 0);
	graph_fill_arc(mask, 0, mask->h-1, r, 0, 90, 0xffffffff);
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, frame_g->w-mask->w, 0, mask->w, mask->h);

	graph_clear(mask, 0);
	graph_fill_arc(mask, 0, 0, r, 270, 360, 0xffffffff);
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, frame_g->w-mask->w, frame_g->h-mask->h, mask->w, mask->h);

	graph_free(mask);
}

void EwokWM::drawFrame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	if((info->style & XWIN_STYLE_NO_TITLE) == 0) {
		//graph_box(frame_g, r->x, r->y, r->w, xwm.theme.titleH+xwm.theme.frameW, fg);
	}

	int round = 13;
	markFrameRound(frame_g, round+1);
	//graph_round_3d(frame_g, 0, 0, frame_g->w, frame_g->h, round+1, 1, bg, false);
	if(xwm.theme.frameW > 0)
		graph_round(frame_g, 0, 0, frame_g->w, frame_g->h, round, xwm.theme.frameW, bg);
}

void EwokWM::drawTitle(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	gsize_t sz;
	font_text_size(info->title, font, xwm.theme.fontSize, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	
	/*grect_t rect;
	getTitle(info, &rect);
	*/

	int pw = (r->w-sz.w)/2;
	int ph = (r->h-sz.h)/2;
	graph_fill(g, r->x, r->y, r->w, r->h, bg);//title box
	//if(top)
		//drawTitlePattern(g, r->x, r->y, r->w, r->h, fg);

	//graph_fill(g, r->x+pw-2, r->y, sz.w+4, r->h, bg);//title box
	graph_draw_text_font(g, r->x+pw, r->y+ph, info->title, font, xwm.theme.fontSize, fg);//title
	//graph_line(g, r->x, r->y+r->h-1, r->x+r->w, r->y+r->h-1, fg);//title box
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

void EwokWM::getClose(xinfo_t* info, grect_t* rect) {
	rect->x = xwm.theme.frameW;
	rect->y = xwm.theme.frameW;// - titleH;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}

void EwokWM::getMax(xinfo_t* info, grect_t* rect) {
	rect->x = xwm.theme.frameW + xwm.theme.titleH;
	rect->y = xwm.theme.frameW;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}


void EwokWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	if(!top)
		return;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	graph_line(g, 
			r->x + r->w - xwm.theme.frameW + 1, r->y,
			r->x + r->w, r->y, dark);
	graph_line(g,
			r->x + r->w - xwm.theme.frameW + 1, r->y + 1,
			r->x + r->w, r->y + 1, bright);
	graph_line(g,
			r->x, r->y + r->h - xwm.theme.frameW + 1,
			r->x, r->y + r->h, dark);
	graph_line(g,
			r->x + 1, r->y + r->h - xwm.theme.frameW + 1,
			r->x + 1, r->y + r->h, bright);
}

void EwokWM::drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
}

void EwokWM::drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill_circle(g, r->x+r->w/2, r->y+r->h/2, r->w/2-3, 0xff66aa22);
	//graph_circle_3d(g, r->x+r->w/2, r->y+r->h/2, r->w/2-3, 1, bg, true);
}

void EwokWM::drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill_circle(g, r->x+r->w/2, r->y+r->h/2, r->w/2-3, 0xffdd6666);
	//graph_circle_3d(g, r->x+r->w/2, r->y+r->h/2, r->w/2-3, 1, bg, true);
}

EwokWM::~EwokWM(void) {
}

EwokWM::EwokWM(void) {
	xwm.theme.desktopBGColor = 0xff555588;
	xwm.theme.desktopFGColor = 0xff8888aa;
	xwm.theme.frameBGColor = 0xffaaaaaa;
	xwm.theme.frameFGColor = 0xff222222;
	xwm.theme.titleH = 32;
}