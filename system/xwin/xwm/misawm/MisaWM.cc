#include "MisaWM.h"
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <graph/graph_ex.h>
#include <graph/graph_png.h>
#include <x++/X.h>
#include <stdlib.h>
#include <string.h>

using namespace Ewok;

/*the tint laid over the frost. The RGB comes from the theme's frame background
  so a theme can colour the glass; the alpha is fixed here and is what makes
  the border semi-transparent. It is deliberately focus-independent: our own
  last output must be a predictable function of the frost alone, otherwise
  ensureFrost could not tell our pixels apart from fresh backdrop. graph_pixel
  keeps the opaque alpha the backdrop copy brought in, so the compositor still
  takes its opaque-ring copy path.*/
inline uint32_t MisaWM::glassTint(void) {
	uint32_t fg, bg;
	getColor(&fg, &bg, true);

	if(color_a(bg) == 255)
		return (bg & 0x88ffffff);
	return bg;
}

/*what graph_pixel(tint) writes over an opaque backdrop pixel: the exact value
  our glass leaves on the display, used to recognise our own output again*/
inline uint32_t MisaWM::blendPixel(uint32_t tint, uint32_t dst) {
	uint32_t a = color_a(tint);
	if(a == 0)
		return dst;
	uint32_t inv_a = 255 - a;
	uint32_t r = (color_r(tint)*a + color_r(dst)*inv_a) / 255;
	uint32_t g = (color_g(tint)*a + color_g(dst)*inv_a) / 255;
	uint32_t b = (color_b(tint)*a + color_b(dst)*inv_a) / 255;
	return argb(0xff, r, g, b);
}

inline bool MisaWM::decontam(graph_t* sharp, graph_t* pred, graph_t* desktop_g, xinfo_t* info,
		int gx, int gy, int gw, int gh, uint32_t tint) {
	if(gw <= 0 || gh <= 0)
		return false;
	int dw = desktop_g->w;
	bool changed = false;
	for(int py = gy; py < gy+gh; py++) {
		if(py < 0 || py >= sharp->h || info->winr.y+py < 0 || info->winr.y+py >= desktop_g->h)
			continue;
		for(int px = gx; px < gx+gw; px++) {
			if(px < 0 || px >= sharp->w || info->winr.x+px < 0 || info->winr.x+px >= desktop_g->w)
				continue;
			uint32_t disp = desktop_g->buffer[(info->winr.y+py)*dw + (info->winr.x+px)];
			uint32_t own = blendPixel(tint, pred->buffer[py*pred->w + px]);
			if(disp != own) {
				sharp->buffer[py*sharp->w + px] = disp; //painted by someone else: fresh backdrop
				changed = true;
			}
			/*else: still our own glass from last frame, carries no backdrop
			  information; keep the backdrop already cached for this pixel*/
		}
	}
	return changed;
}

MisaWM::FrostSlot* MisaWM::findFrost(ewokos_addr_t win) {
	if(win == 0)
		return NULL;
	for(int i = 0; i < MISA_FROST_SLOTS; i++)
		if(frostSlots[i].win == win)
			return &frostSlots[i];
	return NULL;
}

void MisaWM::frostSlotFree(FrostSlot* s) {
	if(s->backdropSharp != NULL) { graph_free(s->backdropSharp); s->backdropSharp = NULL; }
	if(s->frostCache != NULL) { graph_free(s->frostCache); s->frostCache = NULL; }
	s->win = 0;
	s->frostX = s->frostY = s->frostW = s->frostH = 0;
	s->frostValid = false;
	s->lastUse = 0;
	s->lastDraw = 0;
}

MisaWM::FrostSlot* MisaWM::frostSlotFor(ewokos_addr_t win) {
	FrostSlot* slot = findFrost(win);
	if(slot == NULL) {
		/*claim an empty slot, else evict (and free) the least-recently-used one
		  so a busy desktop cannot pin more than MISA_FROST_SLOTS contig pairs*/
		for(int i = 0; i < MISA_FROST_SLOTS; i++) {
			if(frostSlots[i].win == 0) { slot = &frostSlots[i]; break; }
		}
		if(slot == NULL) {
			slot = &frostSlots[0];
			for(int i = 1; i < MISA_FROST_SLOTS; i++)
				if(frostSlots[i].lastUse < slot->lastUse)
					slot = &frostSlots[i];
			frostSlotFree(slot);
		}
		slot->win = win;
	}
	slot->lastUse = ++frostTick;
	return slot;
}

void MisaWM::ensureFrost(graph_t* desktop_g, xinfo_t* info) {
	/*the shadow bands only exist in winr where getWinSpace reserved them: a
	  maximized window carries none (edge to edge), and a frameless one
	  (XWIN_STYLE_NO_FRAME, like the statusbar) carries none either - its winr
	  is the workspace rect as is. Keeping the theme shadow there would size
	  the cache short of the real window and make frostRegion clip every glass
	  region (the title band included) shadow pixels short of the right edge,
	  leaving that strip unfrosted*/
	int shadow = ((info->style & XWIN_STYLE_NO_FRAME) == 0 &&
			info->state != XWIN_STATE_MAX) ? (int)xwm.theme.shadow : 0;
	int fw = (int)info->winr.w - shadow;
	int fh = (int)info->winr.h - shadow;
	if(fw <= 0 || fh <= 0)
		return;
	if(desktop_g == NULL || desktop_g->buffer == NULL)
		return;

	/*Only the pixels the glass actually shows need to be kept blurred. With a
	  1px frame edge (frostRegion early-returns on gw/gh<=1) and no client-area
	  ws_blur, the ONLY region ever read back is the title strip drawn by
	  drawTitle, so maintaining (memcmp + copy + blur) the whole window every
	  frame is ~95% waste - and ensureFrost runs twice per DRAW_FRAME (drawTitle
	  and drawFrame both call it). Limit the maintained height to the title strip
	  plus the blur bleed unless the client area is glass (alpha + ws_blur) or the
	  frame edge is thick enough to be read, both of which still need the full
	  window.*/
	int fhM = fh;
	if(xwm.theme.frameW <= 1 && !(info->alpha && xwm.theme.wsBlur > 0)) {
		fhM = (int)xwm.theme.titleH + 2*(int)xwm.theme.frameBlur + 2;
		if(fhM > fh)
			fhM = fh;
		if(fhM < 1)
			fhM = fh;
	}

	/*this window's own frost slot: stable across frames, so a full-screen
	  rebuild no longer frees/re-allocates the contig canvases per window*/
	FrostSlot* slot = frostSlotFor(info->win);
	if(slot == NULL)
		return;
	if(xwm.draw.serial != 0 && slot->lastDraw == xwm.draw.serial)
		return;
	slot->lastDraw = xwm.draw.serial;
	bool sameSource = slot->sourceId == xwm.draw.backdrop_id &&
			slot->sourceW == desktop_g->w && slot->sourceH == desktop_g->h &&
			slot->alpha == info->alpha &&
			memcmp(&slot->theme, &xwm.theme, sizeof(xwm.theme)) == 0;
	if(!sameSource)
		slot->frostValid = false;
	slot->sourceId = xwm.draw.backdrop_id;
	slot->sourceW = desktop_g->w;
	slot->sourceH = desktop_g->h;
	slot->theme = xwm.theme;
	slot->alpha = info->alpha;
	graph_t* backdropSharp = slot->backdropSharp;
	graph_t* frostCache = slot->frostCache;

	if(desktop_g->w == (int)info->winr.w && desktop_g->h == (int)info->winr.h) {
		/*xserverd handed us the clean window-local snapshot of everything
		  below the window (theme frame_blur): it never carries our own glass,
		  so it can be blurred straight away. Re-blur only when the snapshot
		  actually changed, so idle frames cost a compare only. Only the top fhM
		  rows (the region the glass reads) are kept, compared and blurred.*/
		bool sameSize = backdropSharp != NULL && frostCache != NULL &&
				backdropSharp->w == fw && backdropSharp->h == fhM &&
				frostCache->w == fw && frostCache->h == fhM;
		if(!sameSize) {
			if(backdropSharp != NULL) { graph_free(backdropSharp); backdropSharp = NULL; }
			if(frostCache != NULL) { graph_free(frostCache); frostCache = NULL; }
			slot->frostValid = false;
			backdropSharp = graph_new_shm(fw, fhM);
			frostCache = graph_new_shm(fw, fhM);
			slot->backdropSharp = backdropSharp;
			slot->frostCache = frostCache;
			if(backdropSharp == NULL || frostCache == NULL) {
				if(backdropSharp != NULL) { graph_free(backdropSharp); slot->backdropSharp = NULL; }
				if(frostCache != NULL) { graph_free(frostCache); slot->frostCache = NULL; }
				return;
			}
		}
		if(sameSize && slot->frostValid &&
				slot->frostX == info->winr.x && slot->frostY == info->winr.y) {
			if(xwm.draw.frame.clean_snapshot) {
				if(slot->generation == xwm.draw.frame.backdrop_generation)
					return;
			}
			else if(info->backdrop_unchanged)
				return;
		}
		slot->generation = xwm.draw.frame.backdrop_generation;
		/*Refresh both caches via graph_blt (routed to g2dd) rather than an
		  uncached full-window CPU copy, then blur FROM the sharp snapshot,
		  never from the previous blur.*/
		graph_blt(desktop_g, 0, 0, fw, fhM, backdropSharp, 0, 0, fw, fhM);
		graph_blt(backdropSharp, 0, 0, fw, fhM, frostCache, 0, 0, fw, fhM);
		graph_gaussian_blur(frostCache, 0, 0, fw, fhM, xwm.theme.frameBlur);
		slot->frostX = info->winr.x;
		slot->frostY = info->winr.y;
		slot->frostW = fw;
		slot->frostH = fhM;
		slot->frostValid = true;
		return;
	}

	bool same = slot->frostValid && frostCache != NULL && backdropSharp != NULL &&
			slot->frostX == info->winr.x && slot->frostY == info->winr.y &&
			slot->frostW == fw && slot->frostH == fh;
	if(same) {
		/*same placement: the display under the glass still holds our own last
		  output, except where a window below (or the desktop) repainted over
		  it. Accept only those overwritten pixels as backdrop and keep the
		  cached backdrop elsewhere, so our tint is never read back in.*/
		uint32_t tint = glassTint();
		int wd = (int)xwm.theme.frameW;
		if(wd <= 0)
			wd = 2;
		int th = (int)xwm.theme.titleH;
		bool changed = false;
		changed |= decontam(backdropSharp, frostCache, desktop_g, info, 0, 0, fw, wd, tint);             //top
		changed |= decontam(backdropSharp, frostCache, desktop_g, info, 0, fh-wd, fw, wd, tint);        //bottom
		changed |= decontam(backdropSharp, frostCache, desktop_g, info, 0, wd, wd, fh-2*wd, tint);      //left
		changed |= decontam(backdropSharp, frostCache, desktop_g, info, fw-wd, wd, wd, fh-2*wd, tint);  //right
		/*title band: refresh everywhere except under the opaque paint (the two
		  buttons and the centred title text), which would otherwise be read
		  back as backdrop and ghost into the glass*/
		gsize_t sz;
		font_text_size(info->title, font, xwm.theme.fontSize, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
		int tx = (fw - (int)sz.w)/2;
		int strip = (th - (int)sz.h)/2;
		int bx = wd + 2*th; //right edge of the max button
		changed |= decontam(backdropSharp, frostCache, desktop_g, info, 0, wd, fw, strip, tint);
		changed |= decontam(backdropSharp, frostCache, desktop_g, info, bx, wd+strip, tx-bx, (int)sz.h, tint);
		changed |= decontam(backdropSharp, frostCache, desktop_g, info, tx+(int)sz.w, wd+strip, tx, (int)sz.h, tint);
		changed |= decontam(backdropSharp, frostCache, desktop_g, info, 0, wd+strip+(int)sz.h, fw, th-strip-(int)sz.h, tint);
		if(changed) {
			/*blur FROM the sharp backdrop, never from the previous blur, so the
			  frost cannot accumulate smear frame over frame*/
			memcpy(frostCache->buffer, backdropSharp->buffer, (size_t)fw*fh*4);
			graph_gaussian_blur(frostCache, 0, 0, fw, fh, xwm.theme.frameBlur);
		}
		return;
	}

	if(frostCache != NULL && (frostCache->w != fw || frostCache->h != fh)) {
		graph_free(frostCache);
		frostCache = NULL;
	}
	if(backdropSharp != NULL && (backdropSharp->w != fw || backdropSharp->h != fh)) {
		graph_free(backdropSharp);
		backdropSharp = NULL;
	}
	slot->frostCache = frostCache;
	slot->backdropSharp = backdropSharp;
	slot->frostValid = false;

	/*fresh placement: the desktop is composited bottom-to-top, so the display
	  under the window holds only what is really behind it.
	  frostCache must be shm-backed (graph_new_shm): only those canvases can
	  route to the g2d device, a plain graph_new here blurs the whole window
	  on the cpu every placement.*/
	if(backdropSharp == NULL) backdropSharp = graph_new_shm(fw, fh);
	if(frostCache == NULL) frostCache = graph_new_shm(fw, fh);
	slot->backdropSharp = backdropSharp;
	slot->frostCache = frostCache;
	if(backdropSharp == NULL || frostCache == NULL) {
		if(backdropSharp != NULL) { graph_free(backdropSharp); slot->backdropSharp = NULL; }
		if(frostCache != NULL) { graph_free(frostCache); slot->frostCache = NULL; }
		return;
	}
	graph_blt(desktop_g, info->winr.x, info->winr.y, fw, fh,
			backdropSharp, 0, 0, fw, fh);
	memcpy(frostCache->buffer, backdropSharp->buffer, (size_t)fw*fh*4);
	graph_gaussian_blur(frostCache, 0, 0, fw, fh, xwm.theme.frameBlur);
	slot->frostX = info->winr.x;
	slot->frostY = info->winr.y;
	slot->frostW = fw;
	slot->frostH = fh;
	slot->frostValid = true;
}

void MisaWM::frostRegion(graph_t* desktop_g, graph_t* frame_g, xinfo_t* info,
		int gx, int gy, int gw, int gh, uint32_t tint, int blur) {
	(void)blur; //the blur already happened when the snapshot was taken
	if(gw <= 1 || gh <= 1)
		return;
	if(frame_g == NULL || frame_g->buffer == NULL)
		return;

	/*ensureFrost is hoisted to the callers (drawTitle/drawFrame): it does a
	  whole-window backdrop memcmp, and running it per frosted band scanned
	  the full window six times per repaint even though only the first pass
	  can ever find a change. frostCache is already valid here.*/
	(void)desktop_g;
	FrostSlot* slot = findFrost(info->win);
	if(slot == NULL || slot->frostCache == NULL)
		return;
	/*Clip the region to what the cache actually holds, never read past it.
	  ensureFrost sizes the cache to (winr minus the shadow bands - the whole
	  winr when maximized, where no bands are reserved) and, when the client
	  area is not glass (frame_width<=1, ws_blur=0), to just the title strip. A
	  fullscreen window (NO_FRAME, no shadow band) still asks for its whole
	  client area, which overshoots the cache by the shadow margin - bailing on
	  that left the entire fullscreen glass unfrosted. Clamp instead so the
	  frost still lands everywhere the cache covers; only a fully-outside or
	  degenerate rect is dropped.*/
	if(gx < 0) { gw += gx; gx = 0; }
	if(gy < 0) { gh += gy; gy = 0; }
	if(gx + gw > slot->frostCache->w) gw = slot->frostCache->w - gx;
	if(gy + gh > slot->frostCache->h) gh = slot->frostCache->h - gy;
	if(gw <= 1 || gh <= 1)
		return;

	/*lay the cached frost into the frame, then wash it with a constant
	  translucent tint. Neither step reads the live display, so a content
	  update cannot feed the previous frame back into the glass.*/
	graph_blt(slot->frostCache, gx, gy, gw, gh, frame_g, gx, gy, gw, gh);
	/*the tint is a CONSTANT translucent color laid src-over the frost, so it
	  is exactly a fill-with-alpha: graph_fill_rect routes color_a != 0xff to
	  the g2d fill_alpha primitive, which moves the whole-client-area ws_blur
	  wash onto g2dd/GPU instead of a per-pixel loop on the WM's cpu (regions
	  below G2D_MIN_SIZE fall back to the arch NEON blend, still faster than
	  the old scalar loop). the device blend (g2d_blend_argb_scalar) is
	  formula-identical to the div255 loop this replaces over the opaque
	  backdrop the frost blit just laid down, so the glass is unchanged.*/
	graph_fill_rect(frame_g, gx, gy, gw, gh, tint);
}

void MisaWM::drawDragFrame(graph_t* g, grect_t* r) {
	int wd = xwm.theme.frameW;
	if(wd <= 0)
		wd = 2;
	graph_frame(g, r->x-wd, r->y-wd,
			r->w+wd*2, r->h+wd*2, wd, 0x88ffffff, false);
}

void MisaWM::markFrameRound(graph_t* frame_g, grect_t* fr, int r) {
	if(r <= 0)
		r = 10;

	if(roundMaskSize != r) {
		for(int i = 0; i < 4; i++) {
			if(roundMask[i] != NULL) graph_free(roundMask[i]);
			roundMask[i] = NULL;
		}
		roundMaskSize = r;
	}
	for(int i = 0; i < 4; i++) {
		if(roundMask[i] != NULL)
			continue;
		roundMask[i] = graph_new(NULL, r, r);
		if(roundMask[i] == NULL || roundMask[i]->buffer == NULL)
			return;
		graph_clear(roundMask[i], 0);
		const int cx[4] = {r, r, 0, 0}, cy[4] = {r, 0, r, 0};
		const int start[4] = {90, 180, 0, 270};
		graph_fill_arc(roundMask[i], cx[i], cy[i], r, start[i], start[i]+90, 0xffffffff);
	}

	graph_t* mask = roundMask[0];

	/*the corners of the frame rect fr, not of frame_g: the graph also holds
	  the right/bottom shadow bands, masking at its edges would push the
	  right/bottom corners into the shadow area*/
	mask = roundMask[0];
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, fr->x, fr->y, mask->w, mask->h);

	mask = roundMask[1];
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, fr->x, fr->y+fr->h-mask->h, mask->w, mask->h);

	mask = roundMask[2];
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, fr->x+fr->w-mask->w, fr->y, mask->w, mask->h);

	mask = roundMask[3];
	graph_blt_alpha_mask(mask, 0, 0, mask->w, mask->h, frame_g, fr->x+fr->w-mask->w, fr->y+fr->h-mask->h, mask->w, mask->h);
}

void MisaWM::drawFrame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top) {
	/*corners_only: only this window's own workspace content changed, so xserverd
	  left the decoration ring already sitting in frame_g intact (it skipped
	  clear_frame_ring) and re-blitted just the fresh content over the client
	  area. That blit overwrote the rounded corner pixels where the radius reaches
	  into the workspace, so all that is left is to re-cut the corners and redraw
	  the 3D border arc. The frost cache, the four border bands, the title, the
	  buttons and the client-area glass are all still valid and must NOT be
	  touched: re-blending the translucent glass over itself would darken it.*/
	bool contentOnly = xwm.draw.frame.content_only != 0;
	grect_t savedClip = frame_g->clip;
	if(info->corners_only || contentOnly) {
		grect_t ws = {info->wsr.x-info->winr.x, info->wsr.y-info->winr.y,
			info->wsr.w, info->wsr.h};
		grect_t bounds = {0, 0, frame_g->w, frame_g->h};
		if(!grect_insect(&bounds, &ws) ||
				(savedClip.w > 0 && savedClip.h > 0 && !grect_insect(&savedClip, &ws)))
			return;
		frame_g->clip = ws;
	}
	if(info->corners_only) {
		if((info->style & XWIN_STYLE_NO_FRAME) == 0) {
			int round = (int)xwm.theme.round;
			if(round > 0) {
				markFrameRound(frame_g, r, round);
				graph_round_3d(frame_g, r->x, r->y, r->w, r->h, round, 1, xwm.theme.frameBGColor, false);
			}
		}
		XWM::drawShadow(desktop_g, frame_g, info, top);
		frame_g->clip = savedClip;
		return;
	}

	int fw = r->w;
	int fh = r->h;
	int wd = (int)xwm.theme.frameW;
	if(wd <= 0)
		wd = 2;

	uint32_t tint = glassTint();

	/*ensure the frost once for the whole frame: the four border bands and the
	  optional client-area band below all blit from the same cache, so a single
	  backdrop scan here replaces the five this method used to trigger.*/
	ensureFrost(desktop_g, info);

    if(!contentOnly && (info->style & XWIN_STYLE_NO_FRAME) == 0) {
		/*the border ring: frost the four bands that make up the frame edge. The
		title band is frosted in drawTitle, which runs before this and already
		carries the title text and the buttons, so the ring stops at the title
		and never paints over it.*/
		frostRegion(desktop_g, frame_g, info, r->x, r->y, fw, wd, tint, xwm.theme.frameBlur);               //top
		frostRegion(desktop_g, frame_g, info, r->x, r->y+fh-wd, fw, wd, tint, xwm.theme.frameBlur);         //bottom
		frostRegion(desktop_g, frame_g, info, r->x, r->y+wd, wd, fh-wd*2, tint, xwm.theme.frameBlur);       //left
		frostRegion(desktop_g, frame_g, info, r->x+fw-wd, r->y+wd, wd, fh-wd*2, tint, xwm.theme.frameBlur); //right
	}

	/*the client area is glass too: the whole window sits on one frosted
	  sheet, so frost the wsr as well and lay the client's published buffer
	  back over it with alpha. Opaque content covers the glass completely and
	  looks as before; translucent content now blends over the blurred
	  backdrop instead of the raw copy prepare_win_content put here.*/
	if(info->alpha && xwm.theme.wsBlur > 0 && (info->style & XWIN_STYLE_NO_WS_EFFECT) == 0) {
		grect_t ws = {(int)info->wsr.x - (int)info->winr.x, (int)info->wsr.y - (int)info->winr.y,
				(int)info->wsr.w, (int)info->wsr.h};
		grect_t fr = {r->x, r->y, fw, fh};
		if(grect_insect(&fr, &ws) && ws.w > 0 && ws.h > 0) {
			frostRegion(desktop_g, frame_g, info, ws.x, ws.y, ws.w, ws.h, tint, xwm.theme.wsBlur);
			if(ws_g != NULL && ws_g->buffer != NULL) {
				int sx = ws.x - ((int)info->wsr.x - (int)info->winr.x);
				int sy = ws.y - ((int)info->wsr.y - (int)info->winr.y);
				graph_blt_alpha(ws_g, sx, sy, ws.w, ws.h,
						frame_g, ws.x, ws.y, ws.w, ws.h, 0xff);
			}
		}
	}

    if((info->style & XWIN_STYLE_NO_FRAME) == 0) {
		/*round the corners: the arc mask cuts everything outside the rounded
		corner to transparent, blending the glass edge into the shadow. Same
		geometry ewokwm uses, so the compositor's frame_alpha path applies.*/
		int round = (int)xwm.theme.round;
		if(round > 0) {
			markFrameRound(frame_g, r, round);
			graph_round_3d(frame_g, r->x, r->y, r->w, r->h, round, 1, xwm.theme.frameBGColor, false);
		}
	}
	if(contentOnly) {
		if((info->style & XWIN_STYLE_NO_FRAME) == 0 && info->state != XWIN_STATE_MAX)
			XWM::drawShadow(desktop_g, frame_g, info, top);
		frame_g->clip = savedClip;
	}
}

void MisaWM::drawTitle(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);
	(void)bg;

	/*the title bar is part of the glass sheet: frost it first, then centre
	  the title text on top. This runs before the buttons and before
	  drawFrame, so nothing painted here gets overwritten by the ring.*/
	ensureFrost(desktop_g, info);
	frostRegion(desktop_g, g, info, r->x, r->y, r->w, r->h, glassTint(), xwm.theme.frameBlur);

	gsize_t sz;
	font_text_size(info->title, font, xwm.theme.fontSize, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	int pw = (r->w-sz.w)/2;
	int ph = (r->h-sz.h)/2;
	graph_draw_text_font(g, r->x+pw, r->y+ph, info->title, font, xwm.theme.fontSize, fg);
}

void MisaWM::getClose(xinfo_t* info, grect_t* rect) {
	(void)info;
	rect->x = xwm.theme.frameW;
	rect->y = xwm.theme.frameW;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}

void MisaWM::getMax(xinfo_t* info, grect_t* rect) {
	(void)info;
	rect->x = xwm.theme.frameW + xwm.theme.titleH;
	rect->y = xwm.theme.frameW;
	rect->w = xwm.theme.titleH;
	rect->h = xwm.theme.titleH;
}

void MisaWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
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

void MisaWM::drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)g; (void)info; (void)r; (void)top;
}

void MisaWM::drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info; (void)top;
	graph_fill_circle_3d(g, r->x+r->w/2, r->y+r->h/2, r->w/2-3, 1, 0xff66aa22, false);
}

void MisaWM::drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info; (void)top;
	graph_fill_circle_3d(g, r->x+r->w/2, r->y+r->h/2, r->w/2-3, 1, 0xffdd6666, false);
}

void MisaWM::releaseWindow(ewokos_addr_t win) {
	for(int i = 0; i < MISA_FROST_SLOTS; i++)
		if(win == 0 || frostSlots[i].win == win)
			frostSlotFree(&frostSlots[i]);
}

void MisaWM::updateTheme(bool loadFromX) {
	releaseWindow(0);
	XWM::updateTheme(loadFromX);
}

MisaWM::~MisaWM(void) {
	for(int i = 0; i < 4; i++)
		if(roundMask[i] != NULL) graph_free(roundMask[i]);
	for(int i = 0; i < MISA_FROST_SLOTS; i++)
		frostSlotFree(&frostSlots[i]);
}

MisaWM::MisaWM(void) {
	memset(roundMask, 0, sizeof(roundMask));
	memset(frostSlots, 0, sizeof(frostSlots));
	xwm.retained_decorations = true;
	roundMaskSize = 0;
	for(int i = 0; i < MISA_FROST_SLOTS; i++) {
		frostSlots[i].win = 0;
		frostSlots[i].backdropSharp = NULL;
		frostSlots[i].frostCache = NULL;
		frostSlots[i].frostX = frostSlots[i].frostY = 0;
		frostSlots[i].frostW = frostSlots[i].frostH = 0;
		frostSlots[i].frostValid = false;
		frostSlots[i].lastUse = 0;
	}
	frostTick = 0;
	xwm.theme.desktopBGColor = 0xff555588;
	xwm.theme.desktopFGColor = 0xff8888aa;
	xwm.theme.frameBGColor = 0xffe8eef7;
	xwm.theme.frameFGColor = 0xff202428;
	xwm.theme.titleH = 24;
}
