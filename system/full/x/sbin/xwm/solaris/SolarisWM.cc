#include "SolarisWM.h"
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <upng/upng.h>
#include <sconf/sconf.h>
#include <stdlib.h>

using namespace Ewok;

void SolarisWM::drawDesktop(graph_t* g) {
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

void SolarisWM::drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill_3d(g, r->x, r->y, r->w, r->h, bg, false);
	graph_fill_3d(g, r->x+(r->w/2)-3, r->y+(r->h/2)-3, 6, 6, bg, false);
}

void SolarisWM::drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	graph_fill_3d(g, r->x, r->y, r->w, r->h, bg, false);
	graph_fill_3d(g, r->x+2, r->y+2, r->w-4, r->h-4, bg, false);
}

void SolarisWM::drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill_3d(g, r->x, r->y, r->w, r->h, bg, false);
	graph_fill_3d(g, r->x+5, r->y+(r->h/2)-2,
			r->w-10, 4, bg, false);
}

void SolarisWM::drawDragFrame(graph_t* g, grect_t* r) {
	graph_frame(g, r->x-frameW, r->y-frameW, 
			r->w+frameW*2, r->h+frameW*2, frameW, 0x88ffffff, false);
}

void SolarisWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	if(!top)
		return;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	graph_line(g, 
			r->x + r->w - frameW, r->y,
			r->x + r->w, r->y, dark);
	graph_line(g,
			r->x + r->w - frameW, r->y + 1,
			r->x + r->w, r->y + 1, bright);
	graph_line(g,
			r->x, r->y + r->h - frameW,
			r->x, r->y + r->h, dark);
	graph_line(g,
			r->x + 1, r->y + r->h - frameW,
			r->x + 1, r->y + r->h, bright);
}

void SolarisWM::drawFrame(graph_t* graph, xinfo_t* info, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	int x = info->wsr.x;
	int y = info->wsr.y;
	int w = info->wsr.w;
	int h = info->wsr.h;

	if((info->style & X_STYLE_NO_TITLE) == 0) {
		h += titleH;
		y -= titleH;
	}

	graph_frame(graph, x-frameW, y-frameW, w+frameW*2, h+frameW*2, frameW, bg, false);
	//shadow
	/*if(top) {
		graph_fill(graph, x+w+frameW, y, frameW, h+frameW, 0xaa000000);
		graph_fill(graph, x, y+h+frameW, w+frameW*2, frameW, 0xaa000000);
	}
	*/
}

void SolarisWM::drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	graph_get_3d_color(bg, &dark, &bright);

	gsize_t sz;
	font_text_size(info->title, &font, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	
	int pw = (r->w-sz.w)/2;
	int ph = (r->h-sz.h)/2;
	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_draw_text_font(g, r->x+pw+1, r->y+ph+1, info->title, &font, 0xff222222);//title
	graph_draw_text_font(g, r->x+pw, r->y+ph, info->title, &font, fg);//title
	graph_box_3d(g, r->x, r->y, r->w, r->h, bright, dark);
}

SolarisWM::~SolarisWM(void) {
	font_close(&font);
	if(bgImg == NULL)
		return;
	graph_free(bgImg);
}

SolarisWM::SolarisWM(void) {
}