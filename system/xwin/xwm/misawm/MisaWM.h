#ifndef MISA_WM_H
#define MISA_WM_H

#include <stdio.h>
#include <x++/XWM.h>
#include <font/font.h>

using namespace Ewok;

/*how many windows can hold a cached frost at once. A misawm desktop rarely
  frames more than a handful; the rest are handled by LRU eviction (an evicted
  window simply re-allocates its slot the next time it is drawn). Kept modest so
  the contig shm pool is never exhausted by cached frost canvases.*/
#define MISA_FROST_SLOTS 8

/*MisaWM is ewokwm with a frosted-glass border: the frame edge and the title
  bar show a blurred copy of whatever sits behind the window, washed with a
  translucent tint, instead of a flat opaque color. The rounded corners are
  kept as ewokwm draws them; the drop shadow is the stock graph_shadow_round,
  the window silhouette blurred with the same graph_gaussian_blur the frost uses.*/
class MisaWM : public XWM {

	void markFrameRound(graph_t* frame_g, grect_t* fr, int r);
	/*the constant translucent tint washed over the frost (focus-independent so
	  our own last output is predictable and can be recognised below)*/
	uint32_t glassTint(void);
	/*tint over an opaque backdrop pixel, exactly what graph_pixel writes*/
	uint32_t blendPixel(uint32_t tint, uint32_t dst);
	/*copy rect (gx,gy,gw,gh) of the live display into the sharp backdrop, but
	  skip any pixel that still equals our own last glass output there (it
	  carries no backdrop info); only pixels something else overwrote are
	  fresh backdrop. Returns true when at least one pixel was refreshed.*/
	bool decontam(graph_t* sharp, graph_t* pred, graph_t* desktop_g, xinfo_t* info,
			int gx, int gy, int gw, int gh, uint32_t tint);
	/*(re)build frostCache from the real backdrop behind the window*/
	void ensureFrost(graph_t* desktop_g, xinfo_t* info);
	/*blit the cached frost into frame_g at (gx,gy,gw,gh) and wash it with a
	  constant translucent tint (no display read, so it cannot feed back)*/
	void frostRegion(graph_t* desktop_g, graph_t* frame_g, xinfo_t* info,
			int gx, int gy, int gw, int gh, uint32_t tint, int blur);

	graph_t* roundMask;
	int roundMaskSize;

	/*Per-window frost cache. This used to be a SINGLE shared set of members, so
	  on a full-screen rebuild - which a translucent window forces on nearly
	  every present - xserverd issues DRAW_FRAME for every window in turn, each
	  with a different winr; the shared (frostX,frostY) reuse key never matched
	  the next window, and ensureFrost freed and re-allocated two contig shm
	  canvases (graph_new_shm) plus re-blurred the whole window for EVERY window
	  EVERY frame. That contig alloc/free churn - not the memcpy - was the WM cpu
	  hog, and it defeated the backdrop_unchanged/memcmp skip because the shared
	  sharp copy always belonged to a different window. Keying the cache by
	  xinfo_t.win gives each window a stable slot: allocate once, reuse across
	  frames, and the skip paths fire. The blur is always recomputed FROM the
	  sharp copy, never from the previous blur, so it cannot accumulate smear.*/
	struct FrostSlot {
		ewokos_addr_t win;      //0 = empty slot
		graph_t* backdropSharp; //unblurred backdrop behind the window
		graph_t* frostCache;    //blurred copy the glass is painted from
		int frostX, frostY, frostW, frostH;
		bool frostValid;
		uint32_t lastUse;       //LRU stamp
	};
	FrostSlot frostSlots[MISA_FROST_SLOTS];
	uint32_t frostTick;
	/*find the slot for win without allocating (NULL if absent)*/
	FrostSlot* findFrost(ewokos_addr_t win);
	/*find or claim a slot for win, evicting (and freeing) the LRU one if full*/
	FrostSlot* frostSlotFor(ewokos_addr_t win);
	/*release a slot's canvases and mark it empty*/
	void frostSlotFree(FrostSlot* s);

protected:
	void getMax(xinfo_t* info, grect_t* rect);
	void getClose(xinfo_t* info, grect_t* rect);

	void drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawDragFrame(graph_t* g, grect_t* r);
	void drawFrame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top);
	void drawTitle(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top);
public:
	MisaWM(void);
	~MisaWM(void);
};

#endif
