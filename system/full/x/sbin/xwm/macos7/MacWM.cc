#include "MacWM.h"
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <upng/upng.h>
#include <sconf/sconf.h>
#include <stdlib.h>

using namespace Ewok;

void MacWM::drawDesktop(graph_t* g) {
	XWM::drawDesktop(g);
	if(bgImg == NULL)
		return;
	graph_blt(bgImg, 0, 0, bgImg->w, bgImg->h,
			g, (g->w - bgImg->w)/2, (g->h - bgImg->h)/2, bgImg->w, bgImg->h);
}

void MacWM::drawTitlePattern(graph_t* g, int x, int y, int w, int h, uint32_t fg) {
	int step = 3;
	y = y + step;
	int steps = h / step;

	for (int i = 0; i < steps; i++) {
		graph_line(g, x + 4, y, x + w - 8, y, fg);
		y += step;
	}
}

void MacWM::drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	gsize_t sz;
	get_text_size(info->title, font, &sz.w, &sz.h);
	
	grect_t rect;
	getTitle(info, &rect);

	int pw = (r->w-sz.w)/2;
	graph_fill(g, r->x, r->y, r->w, rect.h, bg);//title box
	if(top) {
		drawTitlePattern(g, r->x, r->y, pw, r->h, fg);
		drawTitlePattern(g, r->x+pw+sz.w, r->y, pw, r->h, fg);
	}
	graph_draw_text(g, r->x+pw, r->y+2, info->title, font, fg);//title
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

	v = sconf_get(sconf, "desktop_color");
	if(v[0] != 0) 
		desktopColor = atoi_base(v, 16);

	sconf_free(sconf);
}

MacWM::~MacWM(void) {
	if(bgImg == NULL)
		return;
	graph_free(bgImg);
}

MacWM::MacWM(void) {
	desktopColor = 0xff555588;
	bgColor = 0xff222222;
	fgColor = 0xff888888;
	bgTopColor = 0xffaaaaaa;
	fgTopColor = 0xff222222;
	bgImg = NULL;
}