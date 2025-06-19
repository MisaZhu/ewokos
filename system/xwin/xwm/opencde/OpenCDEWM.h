#ifndef MACWM_H
#define MACWM_H

#include <stdio.h>
#include <x++/XWM.h>
#include <font/font.h>

using namespace Ewok;

class OpenCDEWM : public XWM {
	void getBorderColor(uint32_t bg, uint32_t *dark, uint32_t *bright);
protected:
	void drawDragFrame(graph_t* g, grect_t* r);
	void drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawFrame(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, grect_t* r, bool top);
	void drawTitle(graph_t* desktop_g, graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawBGEffect(graph_t* desktop_g, graph_t* frame_g, graph_t* ws_g, xinfo_t* info, bool top);
public:
	OpenCDEWM(void);
	~OpenCDEWM(void);
};

#endif