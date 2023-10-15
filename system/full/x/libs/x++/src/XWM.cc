#include "x++/XWM.h"
#include <stdio.h>
#include <string.h>
#include <font/font.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <stdlib.h>

using namespace Ewok;

/*-------get area functions.----------*/
void XWM::getWinSpace(int style, grect_t* xr, grect_t* winr) {
	winr->x = xr->x;
	winr->w = xr->w;

	if((style & X_STYLE_NO_TITLE) == 0 &&
			(style & X_STYLE_NO_FRAME) == 0) {
		winr->y = xr->y - titleH;
		winr->h = xr->h + titleH;
	}
}

static void get_win_space(int style, grect_t* xr, grect_t* winr, void* p) {
	((XWM*)p)->__getWinSpace(style, xr, winr);
}

void XWM::getTitle(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x + titleH;
	rect->y = info->winr.y;// - titleH;

	if((info->style & X_STYLE_NO_RESIZE) == 0)
		rect->w = info->winr.w - titleH*3;
	else
		rect->w = info->winr.w - titleH;

	rect->h = titleH;
}

static void get_title(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getTitle(info, rect);
}

void XWM::getMin(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x+info->winr.w-titleH*2;
	rect->y = info->winr.y;// - titleH;
	rect->w = titleH;
	rect->h = titleH;
}

static void get_min(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getMin(info, rect);
}

void XWM::getMax(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x+info->winr.w-titleH*1;
	rect->y = info->winr.y;// - titleH;
	rect->w = titleH;
	rect->h = titleH;
}

static void get_max(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getMax(info, rect);
}

void XWM::getClose(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x;
	rect->y = info->winr.y;// - titleH;
	rect->w = titleH;
	rect->h = titleH;
}

static void get_close(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getClose(info, rect);
}

void XWM::getResize(xinfo_t* info, grect_t* rect) {
	rect->x = info->wsr.x + info-> wsr.w - 16;
	rect->y = info->wsr.y + info-> wsr.h - 16;
	rect->w = 16 + frameW;
	rect->h = 16 + frameW;
}

static void get_resize(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getResize(info, rect);
}

void XWM::getMinSize(xinfo_t* info, int* w, int* h) {
	(void)info;
	*w = titleH*5;
	*h = titleH*2;
}

static void get_min_size(xinfo_t* info, int* w, int* h, void* p) {
	((XWM*)p)->__getMinSize(info, w, h);
}

/*-------draw functions.----------*/

void XWM::drawDragFrame(graph_t* g, grect_t* r) {
	int x = r->x;
	int y = r->y;
	int w = r->w;
	int h = r->h;

	for(uint32_t i=0; i<frameW; i++) {
		graph_box(g, x-(frameW-i), y-(frameW-i), w+(frameW-i)*2, h+(frameW-i)*2, 0x88000000);
	}
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
		h += titleH;
		//h = titleH;
		y -= titleH;
		graph_box(g, x-frameW, y-frameW, w+frameW*2, titleH+frameW, fg);
	}

	//win box
	for(uint32_t i=0; i<frameW; i++) {
		graph_box(g, x-(frameW-i), y-(frameW-i), w+(frameW-i)*2, h+(frameW-i)*2, fg);
	}
}

static void draw_frame(graph_t* g, xinfo_t* info, bool top, void* p) {
	((XWM*)p)->__drawFrame(g, info, top);
}

void XWM::drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	gsize_t sz;

	int pw = (r->w-sz.w)/2;
	graph_fill(g, r->x, r->y, r->w, titleH, bg);//title box
	graph_box(g, r->x, r->y, r->w, titleH, fg);//title box
}

static void draw_title(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawTitle(g, info, r, top);
}

void XWM::drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box(g, r->x+4, r->y+r->h-8, r->w-8, 4, fg);
}

static void draw_min(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawMin(g, info, r, top);
}

void XWM::drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box(g, r->x+4, r->y+4, r->w-8, r->h-8, fg);
	graph_box(g, r->x+4, r->y+4, r->w-10, r->h-10, fg);
}

static void draw_max(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawMax(g, info, r, top);
}

void XWM::drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box(g, r->x+4, r->y+4, r->w-8, r->h-8, fg);
}

static void draw_close(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawClose(g, info, r, top);
}

void XWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	if(!top)
		return;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box(g, r->x+2, r->y+2, r->w-4, r->h-4, fg);
	graph_box(g, r->x, r->y, r->w, r->h, fg);
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

void XWM::getColor(uint32_t *fg, uint32_t* bg, bool top) {
	if(top) {
		*fg = fgTopColor;
		*bg = bgTopColor;
	}
	else {
		*fg = fgColor;
		*bg = bgColor;
	}
}

void XWM::readConfig(const char* fname) {
	sconf_t *sconf = sconf_load(fname);	
	if(sconf == NULL)
		return;

	const char* v = sconf_get(sconf, "fg_color");
	if(v[0] != 0) 
		fgColor = atoi_base(v, 16);

	v = sconf_get(sconf, "bg_color");
	if(v[0] != 0) 
		bgColor = atoi_base(v, 16);

	v = sconf_get(sconf, "fg_top_color");
	if(v[0] != 0) 
		fgTopColor = atoi_base(v, 16);

	v = sconf_get(sconf, "bg_top_color");
	if(v[0] != 0) 
		bgTopColor = atoi_base(v, 16);

	v = sconf_get(sconf, "desktop_fg_color");
	if(v[0] != 0) 
		desktopFGColor = atoi_base(v, 16);

	v = sconf_get(sconf, "desktop_bg_color");
	if(v[0] != 0) 
		desktopBGColor = atoi_base(v, 16);

	v = sconf_get(sconf, "frame_width");
	if(v[0] != 0) 
		frameW = atoi(v);

	v = sconf_get(sconf, "title_h");
	if(v[0] != 0) 
		titleH = atoi(v);

	int font_size = 14;
	v = sconf_get(sconf, "font_size");
	if(v[0] != 0) 
		font_size = atoi(v);

	v = sconf_get(sconf, "font");
	if(v[0] != 0) 
 		font_load(v, font_size, &font);
	else
 		font_load("/user/system/fonts/system.ttf", font_size, &font);

	sconf_free(sconf);
}

XWM::XWM(void) {
	font_init();
	memset(&xwm, 0, sizeof(xwm_t));

	desktopBGColor = 0xff555588;
	desktopFGColor = 0xff8888aa;
	bgColor = 0xff666666;
	fgColor = 0xff888888;
	bgTopColor = 0xffaaaaaa;
	fgTopColor = 0xff222222;
	frameW = 2;
	titleH = 24;

	xwm.data = this;
	xwm.get_win_space = get_win_space;
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
}

void XWM::run(void) {
	xwm_run(&xwm);
}
