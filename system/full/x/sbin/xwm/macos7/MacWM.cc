#include "MacWM.h"
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <upng/upng.h>
#include <sconf/sconf.h>
#include <stdlib.h>

using namespace Ewok;

void MacWM::drawDesktop(graph_t* g) {
	graph_clear(g, desktopBGColor);
	//background pattern
	int32_t x, y;
	for(y=10; y<(int32_t)g->h; y+=10) {
		for(x=0; x<(int32_t)g->w; x+=10) {
			graph_pixel(g, x, y, desktopFGColor);
		}
	}

	if(bgImg != NULL)
		graph_blt(bgImg, 0, 0, bgImg->w, bgImg->h,
				g, (g->w - bgImg->w)/2, (g->h - bgImg->h)/2, bgImg->w, bgImg->h);
}

void MacWM::drawTitlePattern(graph_t* g, int x, int y, int w, int h, uint32_t fg) {
	int step = 3;
	y = y + step;
	int steps = h / step;

	for (int i = 1; i < steps; i++) {
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
	graph_fill(g, r->x, r->y, r->w, rect.h, bg);//title box
	if(top) {
		drawTitlePattern(g, r->x, r->y, pw, r->h, fg);
		drawTitlePattern(g, r->x+pw+sz.w, r->y, pw, r->h, fg);
	}
	graph_draw_text_font(g, r->x+pw, r->y+2, info->title, &font, fg);//title
}

void MacWM::readConfig(void) {
	sconf_t *sconf = sconf_load("/etc/x/xwm_macos7.conf");	
	if(sconf == NULL)
		return;

	const char* v = sconf_get(sconf, "bg_image");
	if(v[0] != 0) 
		bgImg = png_image_new(v);

	v = sconf_get(sconf, "fg_color");
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

	v = sconf_get(sconf, "title_h");
	if(v[0] != 0) 
		titleH = atoi(v);

	int font_size = 14;
	v = sconf_get(sconf, "font_size");
	if(v[0] != 0) 
		font_size = atoi(v);
 	font_load("/data/fonts/system.ttf", font_size, &font);

	sconf_free(sconf);
}

void MacWM::getColor(uint32_t *fg, uint32_t* bg, bool top) {
	if(top) {
		*fg = fgTopColor;
		*bg = bgTopColor;
	}
	else {
		*fg = fgColor;
		*bg = bgColor;
	}
}

MacWM::~MacWM(void) {
	font_close(&font);
	if(bgImg == NULL)
		return;
	graph_free(bgImg);
}

MacWM::MacWM(void) {
	desktopBGColor = 0xff555588;
	desktopFGColor = 0xff8888aa;
	bgColor = 0xff222222;
	fgColor = 0xff888888;
	bgTopColor = 0xffaaaaaa;
	fgTopColor = 0xff222222;
	bgImg = NULL;
	titleH = 32;
}