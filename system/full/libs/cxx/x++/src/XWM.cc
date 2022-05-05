#include "x++/XWM.h"
#include <stdio.h>
#include <string.h>

using namespace Ewok;

/*-------get area functions.----------*/
static const int TITLE_H = 20;

void XWM::getWorkspace(int style, grect_t* xr, grect_t* wsr) {
	wsr->x = xr->x;
	wsr->w = xr->w;

	if((style & X_STYLE_NO_TITLE) == 0 &&
			(style & X_STYLE_NO_FRAME) == 0) {
		wsr->y = xr->y + TITLE_H;
		wsr->h = xr->h - TITLE_H;
	}
}

static void get_workspace(int style, grect_t* xr, grect_t* wsr, void* p) {
	((XWM*)p)->__getWorkspace(style, xr, wsr);
}

void XWM::getTitle(xinfo_t* info, grect_t* rect) {
	rect->x = info->wsr.x + TITLE_H;
	rect->y = info->wsr.y - TITLE_H;

	if((info->style & X_STYLE_NO_RESIZE) == 0)
		rect->w = info->wsr.w - TITLE_H*3;
	else
		rect->w = info->wsr.w - TITLE_H;

	rect->h = TITLE_H;
}

static void get_title(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getTitle(info, rect);
}

void XWM::getMin(xinfo_t* info, grect_t* rect) {
	rect->x = info->wsr.x+info->wsr.w-TITLE_H*2;
	rect->y = info->wsr.y - TITLE_H;
	rect->w = TITLE_H;
	rect->h = TITLE_H;
}

static void get_min(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getMin(info, rect);
}

void XWM::getMax(xinfo_t* info, grect_t* rect) {
	rect->x = info->wsr.x+info->wsr.w-TITLE_H*1;
	rect->y = info->wsr.y - TITLE_H;
	rect->w = TITLE_H;
	rect->h = TITLE_H;
}

static void get_max(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getMax(info, rect);
}

void XWM::getClose(xinfo_t* info, grect_t* rect) {
	rect->x = info->wsr.x;
	rect->y = info->wsr.y - TITLE_H;
	rect->w = TITLE_H;
	rect->h = TITLE_H;
}

static void get_close(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getClose(info, rect);
}

void XWM::getResize(xinfo_t* info, grect_t* rect) {
	rect->x = info->wsr.x + info-> wsr.w - 16;
	rect->y = info->wsr.y + info-> wsr.h - 16;
	rect->w = 16;
	rect->h = 16;
}

static void get_resize(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getResize(info, rect);
}

void XWM::getMinSize(xinfo_t* info, int* w, int* h) {
	(void)info;
	*w = TITLE_H*5;
	*h = TITLE_H*2;
}

static void get_min_size(xinfo_t* info, int* w, int* h, void* p) {
	((XWM*)p)->__getMinSize(info, w, h);
}

/*-------draw functions.----------*/
void XWM::getColor(uint32_t *fg, uint32_t* bg, bool top) {
	if(!top) {
		*fg = 0xff888888;
		*bg = 0xff222222;
	}
	else {
		*fg = 0xff222222;
		*bg = 0xffaaaaaa;
	}
}

void XWM::drawDragFrame(graph_t* g, grect_t* r) {
	graph_box(g, r->x+1, r->y - TITLE_H+1,
		r->w, r->h + TITLE_H, 0x88000000);
	graph_box(g, r->x, r->y - TITLE_H,
		r->w, r->h + TITLE_H, 0x88ffffff);
}

static void draw_drag_frame(graph_t* g, grect_t* r, void* p) {
	((XWM*)p)->__drawDragFrame(g, r);
}

void XWM::drawFrame(graph_t* g, xinfo_t* info, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	int x = info->wsr.x;
	int y = info->wsr.y;
	int w = info->wsr.w;
	int h = info->wsr.h;
	//int h = 0;

	if((info->style & X_STYLE_NO_TITLE) == 0) {
		h += TITLE_H;
		//h = TITLE_H;
		y -= TITLE_H;
	}
	graph_box(g, x, y, w, h, bg);//win box
	//shadow
	if(top) {
		graph_fill(g, x+w, y+2, 2, h, 0x88000000);
		graph_fill(g, x+2, y+h, w-2, 2, 0x88000000);
	}
}

static void draw_frame(graph_t* g, xinfo_t* info, bool top, void* p) {
	((XWM*)p)->__drawFrame(g, info, top);
}

void XWM::drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	gsize_t sz;
	get_text_size(info->title, font, &sz.w, &sz.h);

	int pw = (r->w-sz.w)/2;
	graph_fill(g, r->x, r->y, r->w, TITLE_H, bg);//title box
	graph_draw_text(g, r->x+pw, r->y+2, info->title, font, fg);//title
}

static void draw_title(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawTitle(g, info, r, top);
}

void XWM::drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box(g, r->x+2, r->y+r->h-6, r->w-4, 4, fg);
}

static void draw_min(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawMin(g, info, r, top);
}

void XWM::drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box(g, r->x+2, r->y+2, r->w-4, r->h-4, fg);
	graph_box(g, r->x+2, r->y+2, r->w-8, r->h-8, fg);
}

static void draw_max(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawMax(g, info, r, top);
}

void XWM::drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box(g, r->x+2, r->y+2, r->w-4, r->h-4, fg);
}

static void draw_close(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawClose(g, info, r, top);
}

void XWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box(g, r->x+2, r->y+2, r->w-4, r->h-4, fg);
}

static void draw_resize(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawResize(g, info, r, top);
}

void XWM::drawDesktop(graph_t* g) {
	uint32_t desktop_fg_color;
	uint32_t desktop_bg_color;
	desktop_fg_color = 0xff8888aa;
	desktop_bg_color = 0xff555588;

	graph_clear(g, desktop_bg_color);
	//background pattern
	int32_t x, y;
	for(y=10; y<(int32_t)g->h; y+=10) {
		for(x=0; x<(int32_t)g->w; x+=10) {
			graph_pixel(g, x, y, desktop_fg_color);
		}
	}
}

static void draw_desktop(graph_t* g, void* p) {
	((XWM*)p)->__drawDesktop(g);
}

XWM::XWM(void) {
	memset(&xwm, 0, sizeof(xwm_t));
	xwm.data = this;
	xwm.get_workspace = get_workspace;
	xwm.get_close = get_close;
	xwm.get_max = get_max;
	xwm.get_min = get_min;
	xwm.get_title = get_title;
	xwm.get_resize = get_resize;
	xwm.get_min_size = get_min_size;

	xwm.draw_frame = draw_frame;
	xwm.draw_drag_frame = draw_drag_frame;
	xwm.draw_close = draw_close;
	xwm.draw_title = draw_title;
	xwm.draw_min = draw_min;
	xwm.draw_max = draw_max;
	xwm.draw_resize = draw_resize;
	xwm.draw_desktop = draw_desktop;

	font = font_by_name("8x16");
}

void XWM::run(void) {
	xwm_run(&xwm);
}
