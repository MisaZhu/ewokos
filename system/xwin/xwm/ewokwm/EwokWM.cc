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

void EwokWM:: markFrameRound(graph_t* frame_g, grect_t* fr, int r) {
	if(r <= 0)
		r = 10;

	if(roundMask == NULL || roundMaskSize != r) {
		if(roundMask != NULL)
			graph_free(roundMask);
		roundMask = graph_new(NULL, r, r);
		roundMaskSize = (roundMask != NULL) ? r : 0;
	}

	graph_t* mask = roundMask;
	if(mask == NULL)
		return;

	/*the corners of the frame rect fr, not of frame_g: the graph also holds
	  the right/bottom shadow bands, masking at its edges would push the
	  right/bottom corners into the shadow area*/
	graph_clear(mask, 0);
	graph_fill_arc(mask, mask->w, mask->h, r, 90, 180, 0xffffffff);
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, fr->x, fr->y, mask->w, mask->h);

	graph_clear(mask, 0);
	graph_fill_arc(mask, mask->w, 0, r, 180, 270, 0xffffffff);
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, fr->x, fr->y+fr->h-mask->h, mask->w, mask->h);

	graph_clear(mask, 0);
	graph_fill_arc(mask, 0, mask->h, r, 0, 90, 0xffffffff);
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, fr->x+fr->w-mask->w, fr->y, mask->w, mask->h);

	graph_clear(mask, 0);
	graph_fill_arc(mask, 0, 0, r, 270, 360, 0xffffffff);
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, fr->x+fr->w-mask->w, fr->y+fr->h-mask->h, mask->w, mask->h);
}

void EwokWM::drawFrame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	if((info->style & XWIN_STYLE_NO_TITLE) == 0) {
		//graph_rect(frame_g, r->x, r->y, r->w, xwm.theme.titleH+xwm.theme.frameW, fg);
	}

	/*the outermost frameW strip is not covered by the title/content fills
	  (they start at frameW): paint it with the border color first, so the
	  rounded-corner AA of graph_round blends over bg instead of the zeroed
	  black buffer, which showed as dark dots where the arc meets the edges*/
	for(uint32_t i = 0; i < xwm.theme.frameW; i++)
		graph_rect(frame_g, r->x+i, r->y+i, r->w-i*2, r->h-i*2, bg);

	/*the radius is part of the theme: xserverd needs it too, to know which
	  pixels of the frame are translucent and blend only those.
	  r is the frame rect without the shadow bands (getFrame), so mask and
	  border follow it instead of the frame_g extents*/
	int round = (int)xwm.theme.round;
	if(round > 0)
		markFrameRound(frame_g, r, round);
	if(xwm.theme.frameW > 0)
		graph_round(frame_g, r->x, r->y, r->w, r->h, round, xwm.theme.frameW, bg);
}

void EwokWM::drawShadow(graph_t* desktop_g, graph_t* g, xinfo_t* info, bool top) {
	int shadow = (int)xwm.theme.shadow;
	if(shadow <= 0)
		return;

	/*the shadow lives in the right/bottom bands getWinSpace reserved in
	  winr, so the rounded frame itself ends at fw/fh*/
	int fw = (int)info->winr.w - shadow;
	int fh = (int)info->winr.h - shadow;
	int round = (int)xwm.theme.round;
	if(round > fw/2) round = fw/2;
	if(round > fh/2) round = fh/2;

	if(round <= 0) { /*square frame: the stock band shadow fits*/
		XWM::drawShadow(desktop_g, g, info, top);
		return;
	}

	uint32_t color = 0x88000000;
	uint8_t a = color_a(color);

	/*a drop shadow is the window silhouette moved by (shadow,shadow) minus
	  the window itself: a pixel gets shadow when it lies inside the offset
	  rounded rect and outside the window's. The offset silhouette is what
	  clips the crescents at the top-right/bottom-left arcs and rounds the
	  outer edge of the bottom-right wrap, so no hand built band junctions
	  are left to seam or spike. Doubled coordinates keep the pixel centers
	  in integers; ring i = i pixels outside the window edge, fading like
	  graph_shadow does.*/
	int fw2 = fw*2, fh2 = fh*2, r2 = round*2, s2 = shadow*2;
	int rr = r2*r2;
	int x0 = fw - round; if(x0 < 0) x0 = 0;
	int x1 = fw + shadow; if(x1 > g->w) x1 = g->w;
	int y0 = fh - round; if(y0 < 0) y0 = 0;
	int y1 = fh + shadow; if(y1 > g->h) y1 = g->h;

	/*right strip: the shadow columns plus the corner squares left of them,
	  full height down to the bottom shadow rows*/
	for(int px = x0; px < x1; px++) {
		int PX = px*2 + 1;
		int dx = (PX > fw2 - r2) ? PX - (fw2 - r2) : ((PX < r2) ? r2 - PX : 0);
		int ox = (PX > fw2 + s2 - r2) ? PX - (fw2 + s2 - r2)
				: ((PX < s2 + r2) ? s2 + r2 - PX : 0);
		for(int py = 0; py < y1; py++) {
			int PY = py*2 + 1;
			int dy = (PY > fh2 - r2) ? PY - (fh2 - r2) : ((PY < r2) ? r2 - PY : 0);
			int dd = dx*dx + dy*dy;
			if(dd < rr)
				continue; //inside the window itself
			int oy = (PY > fh2 + s2 - r2) ? PY - (fh2 + s2 - r2)
					: ((PY < s2 + r2) ? s2 + r2 - PY : 0);
			if(ox*ox + oy*oy >= rr)
				continue; //outside the offset silhouette

			int i = 0;
			while(i < shadow) {
				int t = r2 + (i+1)*2;
				if(dd < t*t)
					break;
				i++;
			}
			if(i >= shadow)
				continue;
			uint8_t alpha = (uint8_t)(a * (shadow - i) / shadow);
			graph_pixel(g, px, py,
					((uint32_t)alpha << 24) | (color & 0x00ffffff));
		}
	}

	/*bottom strip left of the right corner square (that one is done above)*/
	for(int py = y0; py < y1; py++) {
		int PY = py*2 + 1;
		int dy = (PY > fh2 - r2) ? PY - (fh2 - r2) : ((PY < r2) ? r2 - PY : 0);
		int oy = (PY > fh2 + s2 - r2) ? PY - (fh2 + s2 - r2)
				: ((PY < s2 + r2) ? s2 + r2 - PY : 0);
		for(int px = 0; px < x0; px++) {
			int PX = px*2 + 1;
			int dx = (PX > fw2 - r2) ? PX - (fw2 - r2) : ((PX < r2) ? r2 - PX : 0);
			int dd = dx*dx + dy*dy;
			if(dd < rr)
				continue;
			int ox = (PX > fw2 + s2 - r2) ? PX - (fw2 + s2 - r2)
					: ((PX < s2 + r2) ? s2 + r2 - PX : 0);
			if(ox*ox + oy*oy >= rr)
				continue;

			int i = 0;
			while(i < shadow) {
				int t = r2 + (i+1)*2;
				if(dd < t*t)
					break;
				i++;
			}
			if(i >= shadow)
				continue;
			uint8_t alpha = (uint8_t)(a * (shadow - i) / shadow);
			graph_pixel(g, px, py,
					((uint32_t)alpha << 24) | (color & 0x00ffffff));
		}
	}
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
	graph_fill_rect(g, r->x, r->y, r->w, r->h, bg);//title box
	//if(top)
		//drawTitlePattern(g, r->x, r->y, r->w, r->h, fg);

	//graph_fill_rect(g, r->x+pw-2, r->y, sz.w+4, r->h, bg);//title box
	graph_draw_text_font(g, r->x+pw, r->y+ph, info->title, font, xwm.theme.fontSize, fg);//title
	//graph_line(g, r->x, r->y+r->h-1, r->x+r->w, r->y+r->h-1, fg);//title box
}

/*void MacWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	if(!top)
		return;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill_rect(g, r->x, r->y, r->w, r->h, bg & 0x88ffffff);
	graph_fill_rect(g, r->x+3, r->y+3, r->w-6, r->h-6, 0x88222222);
	graph_line(g, r->x+3, r->y+r->h-4, r->x+r->w-4, r->y+3, bg & 0x88ffffff);
	graph_rect(g, r->x, r->y, r->w, r->h, fg & 0x88ffffff);
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

	graph_fill_circle_3d(g, r->x+r->w/2, r->y+r->h/2, r->w/2-3, 1, 0xff66aa22, false);
}

void EwokWM::drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	graph_fill_circle_3d(g, r->x+r->w/2, r->y+r->h/2, r->w/2-3, 1, 0xffdd6666, false);
}

EwokWM::~EwokWM(void) {
	if(roundMask != NULL)
		graph_free(roundMask);
}

EwokWM::EwokWM(void) {
	roundMask = NULL;
	roundMaskSize = 0;
	xwm.theme.desktopBGColor = 0xff555588;
	xwm.theme.desktopFGColor = 0xff8888aa;
	xwm.theme.frameBGColor = 0xffaaaaaa;
	xwm.theme.frameFGColor = 0xff222222;
	xwm.theme.titleH = 32;
}
