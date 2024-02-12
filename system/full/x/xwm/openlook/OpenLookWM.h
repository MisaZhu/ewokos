#ifndef MACWM_H
#define MACWM_H

#include <stdio.h>
#include <x++/XWM.h>
#include <font/font.h>

using namespace Ewok;

class OpenLookWM : public XWM {
	graph_t* frameTLIcon;
	graph_t* frameTRIcon;
	graph_t* frameBLIcon;
	graph_t* frameBRIcon;
	graph_t* frameCloseIcon;
	graph_t* frameMaxIcon;
protected:
	void drawDragFrame(graph_t* g, grect_t* r);
	void drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawFrame(graph_t* g, xinfo_t* info, bool top);
	void drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top);
public:
	OpenLookWM(void);
	~OpenLookWM(void);
};

#endif