#include "x++/XWM.h"
#include <stdio.h>
#include <string.h>
#include <font/font.h>
#include <tinyjson/tinyjson.h>
#include <graph/graph_png.h>
#include <x/x.h>
#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/klog.h>
#include <sys/shm.h>

using namespace Ewok;

/*-------get area functions.----------*/
void XWM::getWinSpace(int style, grect_t* xr, grect_t* winr) {
	winr->x = xr->x;
	winr->w = xr->w;

	if((style & XWIN_STYLE_NO_TITLE) == 0 &&
			(style & XWIN_STYLE_NO_FRAME) == 0) {
		winr->y = xr->y - xwm.theme.titleH;
		winr->h = xr->h + xwm.theme.titleH;
	}
}

static void get_win_space(int style, grect_t* xr, grect_t* winr, void* p) {
	((XWM*)p)->__getWinSpace(style, xr, winr);
}

void XWM::freeWinGraph(graph_t* g) {
	if(g == NULL || g->buffer == NULL)
		return;
	shmdt((void*)g->buffer);
}

bool XWM::fetchWinGraph(xinfo_t* info, graph_t* g) {
	if(info == NULL || info->g_shm_id == -1)
		return false;
	memset(g, 0, sizeof(graph_t));

	uint32_t* g_shm = (uint32_t*)shmat(info->g_shm_id, 0, 0);
	if(g_shm == NULL)
		return false;
	g->buffer = g_shm;
	g->w = info->wsr.w;
	g->h = info->wsr.h;
	g->need_free = false;
	return true;
}

void XWM::getTitle(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x;
	rect->y = info->winr.y;
	rect->w = info->winr.w;
	rect->h = xwm.theme.titleH;
}

static void get_title(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getTitle(info, rect);
}

void XWM::getMin(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x + info->winr.w - xwm.theme.titleH*2;
	rect->y = info->winr.y;// - titleH;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}

static void get_min(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getMin(info, rect);
}

void XWM::getMax(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x + info->winr.w- xwm.theme.titleH;
	rect->y = info->winr.y;// - titleH;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}

static void get_max(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getMax(info, rect);
}

void XWM::getClose(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.x;
	rect->y = info->winr.y;// - titleH;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}

static void get_close(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getClose(info, rect);
}

void XWM::getResize(xinfo_t* info, grect_t* rect) {
	rect->x = info->wsr.x + info-> wsr.w - 20;
	rect->y = info->wsr.y + info-> wsr.h - 20;
	rect->w = 20 + xwm.theme.frameW - 1;
	rect->h = 20 + xwm.theme.frameW - 1;
}

static void get_resize(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getResize(info, rect);
}

void XWM::getMinSize(xinfo_t* info, int* w, int* h) {
	(void)info;
	*w = xwm.theme.titleH*5;
	*h = xwm.theme.titleH*2;
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

	for(uint32_t i=0; i<xwm.theme.frameW; i++) {
		graph_box(g, x-(xwm.theme.frameW-i), y-(xwm.theme.frameW-i), w+(xwm.theme.frameW-i)*2, h+(xwm.theme.frameW-i)*2, 0x88000000);
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

	if((info->style & XWIN_STYLE_NO_TITLE) == 0) {
		h += xwm.theme.titleH;
		//h = xwm.theme.titleH;
		y -= xwm.theme.titleH;
		graph_box(g, x-xwm.theme.frameW, y-xwm.theme.frameW, w+xwm.theme.frameW*2, xwm.theme.titleH+xwm.theme.frameW, fg);
	}

	//win box
	for(uint32_t i=0; i<xwm.theme.frameW; i++) {
		graph_box(g, x-(xwm.theme.frameW-i), y-(xwm.theme.frameW-i), w+(xwm.theme.frameW-i)*2, h+(xwm.theme.frameW-i)*2, fg);
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
	graph_fill(g, r->x, r->y, r->w, xwm.theme.titleH, bg);//title box
	graph_box(g, r->x, r->y, r->w, xwm.theme.titleH, fg);//title box
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
	graph_box(g, r->x+4, r->y+4, r->w-12, r->h-12, fg);
	graph_box(g, r->x+4, r->y+4, r->w-8, r->h-8, fg);
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
	graph_box(g, r->x+3, r->y+3, r->w-6, r->h-6, fg);
	graph_box(g, r->x, r->y, r->w, r->h, fg);
}

static void draw_resize(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawResize(g, info, r, top);
}

graph_t* XWM::genDesktopPattern(void) {
	graph_t* g = graph_new(NULL, 64, 64);
	graph_draw_dot_pattern(g, 0, 0, g->w, g->h, xwm.theme.desktopBGColor, xwm.theme.desktopFGColor, 2);
	return g;
}

void XWM::drawDesktop(graph_t* g) {
	if(desktopPattern == NULL)
		desktopPattern = genDesktopPattern();
	graph_clear(g, 0xffffffff);

	int x = 0;
	int y = 0;
	for(int i=0; y<g->h; i++) {
		for(int j=0; x<g->w;j++) {
			graph_blt(desktopPattern, 0, 0, desktopPattern->w, desktopPattern->h,
					g, x, y, desktopPattern->w, desktopPattern->h);
			x += desktopPattern->w;
		}
		x = 0;
		y += desktopPattern->h;
	}
}

static void draw_desktop(graph_t* g, void* p) {
	((XWM*)p)->__drawDesktop(g);
}

void XWM::__drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	drawTitle(g, info, r, top);
}

void XWM::getColor(uint32_t *fg, uint32_t* bg, bool top) {
	if(top) {
		*fg = xwm.theme.fgTopColor;
		*bg = xwm.theme.bgTopColor;
	}
	else {
		*fg = xwm.theme.fgColor;
		*bg = xwm.theme.bgColor;
	}
}

void XWM::updateTheme(void) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0)
		return;

	proto_t out;
	PF->init(&out);
	if(dev_cntl_by_pid(xserv_pid, X_DCNTL_GET_XWM_THEME, NULL, &out) != 0) {
		return;
	}	
	proto_read_to(&out, &xwm.theme, sizeof(xwm_theme_t));
	PF->clear(&out);

	if(font != NULL) {
		font_free(font);
		font = NULL;
	}
 	font = font_new(xwm.theme.fontName, true);

	if(desktopPattern != NULL) {
		graph_free(desktopPattern);
		desktopPattern = NULL;
	}
	if(xwm.theme.patternName[0] != 0 && strcmp(xwm.theme.patternName, "none") != 0)
		desktopPattern = png_image_new_bg(x_get_theme_fname(X_THEME_ROOT, "xwm", xwm.theme.patternName), xwm.theme.desktopBGColor);
}

void XWM::loadTheme(const char* name) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0 || name == NULL || name[0] == 0) {
		return;
	}

	proto_t in;
	PF->init(&in)->adds(&in, name);
	
	if(dev_cntl_by_pid(xserv_pid, X_DCNTL_LOAD_THEME, &in, NULL) != 0) {
		return;
	}

	if(dev_cntl_by_pid(xserv_pid, X_DCNTL_LOAD_XWM_THEME, &in, NULL) != 0) {
		return;
	}	
	PF->clear(&in);

	onLoadTheme();
}

XWM::XWM(void) {
	font_init();
	memset(&xwm, 0, sizeof(xwm_t));

	desktopPattern = NULL;
	xwm.theme.desktopBGColor = 0xff555588;
	xwm.theme.desktopFGColor = 0xff8888aa;
	xwm.theme.bgColor = 0xff666666;
	xwm.theme.fgColor = 0xff888888;
	xwm.theme.bgTopColor = 0xffaaaaaa;
	xwm.theme.fgTopColor = 0xff222222;
	xwm.theme.frameW = 2;
	xwm.theme.titleH = 24;
	font = NULL;

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

XWM::~XWM(void) {
	if(desktopPattern != NULL)
		graph_free(desktopPattern);
	if(font != NULL)
		font_free(font);
}

void XWM::run(void) {
	updateTheme();
	xwm_run(&xwm);
}
