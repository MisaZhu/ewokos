#include "OpenCDEWM.h"
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <graph/graph_png.h>
#include <x/x.h>
#include <stdlib.h>
#include <string.h>

using namespace Ewok;

void OpenCDEWM::drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	graph_box_3d(g, r->x, r->y, r->w, r->h, bright, dark);
	graph_box_3d(g, r->x+(r->w/2)-3, r->y+(r->h/2)-3, 6, 6, dark, bright);
}

void OpenCDEWM::drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	graph_box_3d(g, r->x, r->y, r->w, r->h, bright, dark);
	graph_box_3d(g, r->x+3, r->y+3, r->w-6, r->h-6, dark, bright);
}

void OpenCDEWM::drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	graph_box_3d(g, r->x, r->y, r->w, r->h, bright, dark);
	graph_box_3d(g, r->x+5, r->y+(r->h/2)-2,
			r->w-10, 4, dark, bright);
}

void OpenCDEWM::drawDragFrame(graph_t* g, grect_t* r) {
	graph_frame(g, r->x-xwm.theme.frameW, r->y-xwm.theme.frameW, 
			r->w+xwm.theme.frameW*2, r->h+xwm.theme.frameW*2, xwm.theme.frameW, 0x88ffffff, false);
}

void OpenCDEWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
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

enum {
    BG_EFFECT_NONE = 0,
    BG_EFFECT_TRANSPARENT,
    BG_EFFECT_DOT,
    BG_EFFECT_GLASS
};

void OpenCDEWM::drawBGEffect(graph_t* desktop_g, graph_t* frame_g, xinfo_t* info, bool top) {
	if(top || info->anti_bg_effect || xwm.theme.bgEffect == BG_EFFECT_NONE)
		return;
	graph_blt_alpha(frame_g, 0, 0, 
		info->winr.w,
		info->winr.h,
		desktop_g,
		info->winr.x,
		info->winr.y,
		info->winr.w,
		info->winr.h, 0x88);

	switch(xwm.theme.bgEffect) {
		case BG_EFFECT_TRANSPARENT:
			return;
		case BG_EFFECT_DOT:
			graph_draw_dot_pattern(frame_g, 
				0, 0, info->winr.w, info->winr.h,
				0x88ffffff, 0x88000000, 2, 1);	
			return;
		case BG_EFFECT_GLASS:
			graph_glass(desktop_g, info->winr.x, info->winr.y, info->winr.w, info->winr.h, 2);
			graph_blt(desktop_g, info->winr.x, info->winr.y, info->winr.w, info->winr.h, 
				frame_g, 0, 0, info->winr.w, info->winr.h);
			return;
	}
}

void OpenCDEWM::drawFrame(graph_t* desktop_g, graph_t* g, xinfo_t* info, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	int x = 0;
	int y = 0;
	int w = info->winr.w;
	int h = info->winr.h;

	graph_frame(g, x, y, w, h, xwm.theme.frameW, bg, false);
}

void OpenCDEWM::drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	gsize_t sz;
	font_text_size(info->title, font, xwm.theme.fontSize, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	
	int pw = (r->w-sz.w)/2;
	int ph = (r->h-sz.h)/2;

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	if(xwm.theme.alpha && color_a(bg) != 0xFF)
		graph_glass(g, r->x, r->y, r->w, r->h, 2);

	graph_draw_text_font(g, r->x+pw+1, r->y+ph+1, info->title, font, xwm.theme.fontSize, 0xff222222);//title
	graph_draw_text_font(g, r->x+pw, r->y+ph, info->title, font, xwm.theme.fontSize, fg);//title
	graph_box_3d(g, r->x, r->y, r->w, r->h, bright, dark);
}

OpenCDEWM::~OpenCDEWM(void) {
}

OpenCDEWM::OpenCDEWM(void) {
}