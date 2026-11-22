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
	/*corners_only/content_only: only this window's own workspace content
	  changed, so xserverd left the decoration ring already sitting in frame_g
	  intact (it skipped clear_frame_ring) and re-blitted just the fresh
	  content over the client area. That blit overwrote the rounded corner
	  pixels where the radius reaches into the workspace, so all that is left
	  is to re-cut the corners and redraw the border arc. The border bands,
	  title, buttons and shadow are all still valid and must NOT be touched:
	  the base DRAW_FRAME pass already skipped them via keep_decor.*/
	bool contentOnly = xwm.draw.frame.content_only != 0;
	grect_t savedClip = frame_g->clip;
	if(info->corners_only || contentOnly) {
		grect_t ws = {(int)info->wsr.x - (int)info->winr.x, (int)info->wsr.y - (int)info->winr.y,
			(int)info->wsr.w, (int)info->wsr.h};
		/*content_only may carry a damage band: only the corner squares the
		  band reaches were overwritten by the fresh content blit, so clip
		  the re-cut to it (the rest of the ring is untouched)*/
		if(contentOnly && xwm.draw.frame.damage_valid) {
			grect_t d = xwm.draw.frame.damage;
			if(grect_insect(&ws, &d))
				ws = d;
		}
		grect_t bounds = {0, 0, frame_g->w, frame_g->h};
		if(!grect_insect(&bounds, &ws) ||
				(savedClip.w > 0 && savedClip.h > 0 && !grect_insect(&savedClip, &ws)))
			return;
		frame_g->clip = ws;
		if((info->style & XWIN_STYLE_NO_FRAME) == 0) {
			int round = (int)xwm.theme.round;
			if(round > 0) {
				uint32_t fg, bg;
				getColor(&fg, &bg, top);
				markFrameRound(frame_g, r, round);
				if(xwm.theme.frameW > 0)
					graph_round(frame_g, r->x, r->y, r->w, r->h, round, xwm.theme.frameW, bg);
			}
		}
		frame_g->clip = savedClip;
		return;
	}

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
	int frameW = (info->style & XWIN_STYLE_NO_FRAME) ? 0 : (int)xwm.theme.frameW;
	rect->x = frameW;
	rect->y = frameW;// - titleH;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}

void EwokWM::getMax(xinfo_t* info, grect_t* rect) {
	int frameW = (info->style & XWIN_STYLE_NO_FRAME) ? 0 : (int)xwm.theme.frameW;
	rect->x = frameW + xwm.theme.titleH;
	rect->y = frameW;
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
	/*the decoration ring survives content-only presents: xserverd keeps it in
	  frame_g and sends corners_only/content_only instead of clearing the ring,
	  so an alpha window's steady-state repaint skips the title font rendering,
	  buttons, border fills and shadow - the same fast path MisaWM uses*/
	xwm.retained_decorations = true;
	xwm.theme.desktopBGColor = 0xff555588;
	xwm.theme.desktopFGColor = 0xff8888aa;
	xwm.theme.frameBGColor = 0xffaaaaaa;
	xwm.theme.frameFGColor = 0xff222222;
	xwm.theme.titleH = 32;
}
