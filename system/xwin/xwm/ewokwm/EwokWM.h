#ifndef EWOK_WM_H
#define EWOK_WM_H

#include <stdio.h>
#include <x++/XWM.h>
#include <font/font.h>

using namespace Ewok;

class EwokWM : public XWM {
	void drawTitlePattern(graph_t* g, int x, int y, int w, int h, uint32_t fg);
	void markFrameRound(graph_t* frame_g, int r);
	graph_t* roundMask;
	int roundMaskSize;

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
	EwokWM(void);
	~EwokWM(void);
};

#endif
