#ifndef MISA_WM_H
#define MISA_WM_H

#include <stdio.h>
#include <x++/XWM.h>
#include <font/font.h>

using namespace Ewok;

/*MisaWM is ewokwm with a frosted-glass border: the frame edge and the title
  bar show a blurred copy of whatever sits behind the window, washed with a
  translucent tint, instead of a flat opaque color. The rounded corners and
  the drop shadow are kept exactly as ewokwm draws them.*/
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

	/*unblurred backdrop behind the window, and the blurred copy the glass is
	  actually painted from. The blur is always recomputed FROM the sharp
	  copy, never from the previous blur, so it cannot accumulate smear.*/
	graph_t* backdropSharp;
	graph_t* frostCache;
	int frostX, frostY, frostW, frostH;
	bool frostValid;

protected:
	void getMax(xinfo_t* info, grect_t* rect);
	void getClose(xinfo_t* info, grect_t* rect);

	void drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawDragFrame(graph_t* g, grect_t* r);
	void drawFrame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top);
	void drawShadow(graph_t* desktop_g, graph_t* g, xinfo_t* info, bool top);
	void drawTitle(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top);
public:
	MisaWM(void);
	~MisaWM(void);
};

#endif
