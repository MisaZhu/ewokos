#include "x++/XWM.h"
#include <stdio.h>
#include <string.h>
#include <font/font.h>
#include <tinyjson/tinyjson.h>
#include <graph/graph_image.h>
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

	if((style & XWIN_STYLE_NO_FRAME) == 0) {
		winr->x -= xwm.theme.frameW;
		winr->w += 2*xwm.theme.frameW + xwm.theme.shadow;
		winr->y -= xwm.theme.frameW;
		winr->h += 2*xwm.theme.frameW + xwm.theme.shadow;
	}
}

static void get_win_space(int style, grect_t* xr, grect_t* winr, void* p) {
	((XWM*)p)->__getWinSpace(style, xr, winr);
}

void XWM::getTitle(xinfo_t* info, grect_t* rect) {
	rect->x = xwm.theme.frameW;
	rect->y = xwm.theme.frameW;
	rect->w = info->winr.w - xwm.theme.frameW*2 - xwm.theme.shadow;
	rect->h = xwm.theme.titleH;
}

static void get_title(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getTitle(info, rect);
}

void XWM::getMin(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.w - xwm.theme.titleH*2 - xwm.theme.frameW - xwm.theme.shadow;
	rect->y = xwm.theme.frameW;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}

static void get_min(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getMin(info, rect);
}

void XWM::getMax(xinfo_t* info, grect_t* rect) {
	rect->x = info->winr.w- xwm.theme.titleH - xwm.theme.frameW - xwm.theme.shadow;
	rect->y = xwm.theme.frameW;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}

static void get_max(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getMax(info, rect);
}

void XWM::getClose(xinfo_t* info, grect_t* rect) {
	rect->x = xwm.theme.frameW;
	rect->y = xwm.theme.frameW;// - titleH;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}

void XWM::getFrame(xinfo_t* info, grect_t* rect) {
	rect->x = 0;
	rect->y = 0;// - titleH;
	rect->w = info->winr.w - xwm.theme.shadow;
	rect->h = info->winr.h - xwm.theme.shadow;
}

static void get_close(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getClose(info, rect);
}

static void get_frame(xinfo_t* info, grect_t* rect, void* p) {
	((XWM*)p)->__getFrame(info, rect);
}

void XWM::getResize(xinfo_t* info, grect_t* rect) {
	rect->x = info-> winr.w - 20 - xwm.theme.frameW - xwm.theme.shadow;
	rect->y = info-> winr.h - 20 - xwm.theme.frameW - xwm.theme.shadow;
	rect->w = 20 + xwm.theme.frameW;
	rect->h = 20 + xwm.theme.frameW;
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
	int w = r->w - xwm.theme.shadow;
	int h = r->h - xwm.theme.shadow;

	int wd = xwm.theme.frameW;
	if(wd <= 0)
		wd = 2;
	graph_frame(g, r->x-wd, r->y-wd, 
			r->w+wd*2, r->h+wd*2, wd, 0x88ffffff, false);
}

static void draw_drag_frame(graph_t* g, grect_t* r, void* p) {
	((XWM*)p)->__drawDragFrame(g, r);
}

void XWM::drawFrame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	if((info->style & XWIN_STYLE_NO_TITLE) == 0) {
		graph_rect(frame_g, r->x, r->y, r->w, xwm.theme.titleH+xwm.theme.frameW, fg);
	}

	//win box
	for(uint32_t i=0; i<xwm.theme.frameW; i++) {
		graph_rect(frame_g, r->x+i, r->y+i, r->w-i*2, r->h-i*2, fg);
	}
}

static void draw_frame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawFrame(desktop_g, frame_g, ws_g, info, r, top);
}

void XWM::drawShadow(graph_t* desktop_g, graph_t* g, xinfo_t* info, bool top) {
    if(xwm.theme.shadow == 0)
        return;
	uint32_t color = 0x88000000;
	graph_shadow(g, 0, 0, info->winr.w, info->winr.h, xwm.theme.shadow, color);
}

void draw_shadow(graph_t* desktop_g, graph_t* g, xinfo_t* info, bool top, void* p) {
	((XWM*)p)->__drawShadow(desktop_g, g, info, top);
}

void update_theme(bool loadFromX, void* p) {
	((XWM*)p)->__updateTheme(loadFromX);
}

static void draw_bg_effect(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, bool top, void* p) {
	((XWM*)p)->__drawBGEffect(desktop_g, frame_g, ws_g, info, top);
}

void XWM::drawTitle(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	graph_fill_rect(g, r->x, r->y, r->w, xwm.theme.titleH, bg);//title box
	graph_rect(g, r->x, r->y, r->w, xwm.theme.titleH, fg);//title box
}

static void draw_title(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawTitle(desktop_g, g, info, r, top);
}

void XWM::drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill_rect(g, r->x, r->y, r->w, r->h, bg);
	graph_rect(g, r->x+4, r->y+r->h-8, r->w-8, 4, fg);
}

static void draw_min(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawMin(g, info, r, top);
}

void XWM::drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill_rect(g, r->x, r->y, r->w, r->h, bg);
	graph_rect(g, r->x+4, r->y+4, r->w-12, r->h-12, fg);
	graph_rect(g, r->x+4, r->y+4, r->w-8, r->h-8, fg);
}

static void draw_max(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawMax(g, info, r, top);
}

void XWM::drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill_rect(g, r->x, r->y, r->w, r->h, bg);
	graph_rect(g, r->x+4, r->y+4, r->w-8, r->h-8, fg);
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

	graph_fill_rect(g, r->x, r->y, r->w, r->h, bg);
	graph_rect(g, r->x+3, r->y+3, r->w-6, r->h-6, fg);
	graph_rect(g, r->x, r->y, r->w, r->h, fg);
}

static void draw_resize(graph_t* g, xinfo_t* info, grect_t* r, bool top, void* p) {
	((XWM*)p)->__drawResize(g, info, r, top);
}

graph_t* XWM::genDesktopPattern(void) {
	graph_t* g = graph_new(NULL, 64, 64);
	graph_draw_dot_pattern(g, 0, 0, g->w, g->h, xwm.theme.desktopBGColor, xwm.theme.desktopFGColor, 2, 1);
	return g;
}

uint32_t XWM::getPatternMode(graph_t* g, float* scale) {
	if(xwm.theme.desktopPatternMode != DESKTOP_PATTERN_AUTO)
		return xwm.theme.desktopPatternMode;
	
	float sc = 1.0;
	float scaleX = (float)g->w / desktopPattern->w;
	float scaleY = (float)g->h / desktopPattern->h;
	if(scaleX >= 0 || scaleY >= 0)
		sc = (scaleX < scaleY) ? scaleX : scaleY;
	else
		sc = (scaleX > scaleY) ? scaleX : scaleY;
	*scale = sc;

	if(sc < 1.0)
		return DESKTOP_PATTERN_FIT;
	else if(sc > 1.6)
		return DESKTOP_PATTERN_TILE;
	return DESKTOP_PATTERN_CENTER;
}

void XWM::drawDesktop(graph_t* g) {
	uint32_t patternMode = xwm.theme.desktopPatternMode;
	if(desktopPattern == NULL) {
		desktopPattern = genDesktopPattern();
	}
	else {
		float scale = 1.0;
		patternMode = getPatternMode(g, &scale);
		if(scale != 1.0 && !patternFit && patternMode == DESKTOP_PATTERN_FIT) {
			// Calculate scale to fit desktop while maintaining aspect ratio
			// Use the smaller dimension to ensure the pattern fills the entire screen
			graph_t * desktopPatternFit = graph_scalef_fast(desktopPattern, scale);
			if(desktopPatternFit != NULL) {
				graph_free(desktopPattern);
				desktopPattern = desktopPatternFit; 
			}
			patternFit = true;
		}
	}

	graph_clear(g, xwm.theme.desktopBGColor);

	// Calculate center of screen
	int centerX = g->w / 2;
	int centerY = g->h / 2;
	
	// Calculate starting position to center the pattern
	// Align so that a pattern is centered at the screen center
	int startX = centerX - (desktopPattern->w / 2);
	int startY = centerY - (desktopPattern->h / 2);
	
	if(patternMode == DESKTOP_PATTERN_TILE) {
		// Adjust to ensure we cover the entire screen by going backwards first
		while(startX > 0) startX -= desktopPattern->w;
		while(startY > 0) startY -= desktopPattern->h;
		
		// Tile from the adjusted start position to cover entire screen
		for(int y = startY; y < g->h; y += desktopPattern->h) {
			for(int x = startX; x < g->w; x += desktopPattern->w) {
				graph_blt(desktopPattern, 0, 0, desktopPattern->w, desktopPattern->h,
						g, x, y, desktopPattern->w, desktopPattern->h);
			}
		}
	}
	else if(patternMode == DESKTOP_PATTERN_CENTER ||
			patternMode == DESKTOP_PATTERN_FIT) {
		graph_blt(desktopPattern, 0, 0, desktopPattern->w, desktopPattern->h,
				g, startX, startY, desktopPattern->w, desktopPattern->h);
	}
}

static void draw_desktop(graph_t* g, void* p) {
	((XWM*)p)->__drawDesktop(g);
}

void XWM::drawBGEffect(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, bool top) {
	if(top || xwm.theme.bgEffect == BG_EFFECT_NONE)
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
			graph_blt(desktop_g, 
				info->winr.x, info->winr.y, info->winr.w, info->winr.h, 
				frame_g, 0, 0, info->winr.w, info->winr.h);
			return;
		case BG_EFFECT_DOT:
			graph_draw_dot_pattern(desktop_g, 
				info->wsr.x, info->wsr.y, info->wsr.w, info->wsr.h,
				0x33ffffff, 0x33000000, 2, 1);	
			graph_blt(desktop_g, 
				info->winr.x, info->winr.y, info->winr.w, info->winr.h, 
				frame_g, 0, 0, info->winr.w, info->winr.h);
			return;
		case BG_EFFECT_GLASS:
			graph_glass(desktop_g, info->wsr.x, info->wsr.y, info->wsr.w, info->wsr.h, 3);
			graph_blt(desktop_g, 
				info->winr.x, info->winr.y, info->winr.w, info->winr.h, 
				frame_g, 0, 0, info->winr.w, info->winr.h);
			return;
		case BG_EFFECT_GAUSSIAN:
			graph_gaussian(desktop_g, info->wsr.x, info->wsr.y, info->wsr.w, info->wsr.h, 3);
			graph_blt(desktop_g, 
				info->winr.x, info->winr.y, info->winr.w, info->winr.h, 
				frame_g, 0, 0, info->winr.w, info->winr.h);
		return;
	}
}

void XWM::getColor(uint32_t *fg, uint32_t* bg, bool top) {
	if(top) {
		*fg = xwm.theme.frameFGColor;
		*bg = xwm.theme.frameBGColor;
	}
	else {
		*fg = color_gray(xwm.theme.frameFGColor);
		*bg = color_gray(xwm.theme.frameBGColor);
		//*fg = graph_get_dark_color(xwm.theme.frameFGColor);
		//*bg = graph_get_dark_color(xwm.theme.frameBGColor);
	}
}

void XWM::updateTheme(bool loadFromX) {
	if(loadFromX) {
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
	}

	if(font != NULL) {
		font_free(font);
		font = NULL;
	}
 	font = font_new(xwm.theme.fontName, true);

	patternFit = false;
	if(desktopPattern != NULL) {
		graph_free(desktopPattern);
		desktopPattern = NULL;
	}
	if(xwm.theme.patternName[0] != 0 && strcmp(xwm.theme.patternName, "none") != 0) {
		char fname[FS_FULL_NAME_MAX+1] = {0};
		x_get_theme_fname(X_THEME_ROOT, "xwm", xwm.theme.patternName, fname, FS_FULL_NAME_MAX);
		desktopPattern = graph_image_new_bg(fname, xwm.theme.desktopBGColor);
	}
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
	patternFit = false;

	xwm.theme.desktopBGColor = 0xff555588;
	xwm.theme.desktopFGColor = 0xff8888aa;
	xwm.theme.frameBGColor = 0xffaaaaaa;
	xwm.theme.frameFGColor = 0xff222222;
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
	xwm.get_frame = get_frame;

	xwm.draw_bg_effect = draw_bg_effect;
	xwm.draw_frame = draw_frame;
	xwm.draw_drag_frame = draw_drag_frame;
	xwm.draw_close = draw_close;
	xwm.draw_title = draw_title;
	xwm.draw_min = draw_min;
	xwm.draw_max = draw_max;
	xwm.draw_resize = draw_resize;
	xwm.draw_desktop = draw_desktop;
	xwm.draw_shadow = draw_shadow;
	xwm.update_theme = update_theme;
}

XWM::~XWM(void) {
	if(desktopPattern != NULL)
		graph_free(desktopPattern);
	if(font != NULL)
		font_free(font);
	font_quit();
}

void XWM::run(void) {
	updateTheme(true);
	xwm_run(&xwm);
}
