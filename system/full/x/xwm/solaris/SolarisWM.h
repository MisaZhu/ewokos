#ifndef MACWM_H
#define MACWM_H

#include <stdio.h>
#include <x++/XWM.h>
#include <font/font.h>

using namespace Ewok;

class SolarisWM : public XWM {
	graph_t* pattern;
	graph_t* genPattern(void);
	void getBorderColor(uint32_t bg, uint32_t *dark, uint32_t *bright);
protected:
	void drawDragFrame(graph_t* g, grect_t* r);
	void drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawFrame(graph_t* g, xinfo_t* info, bool top);
	void drawDesktop(graph_t* g);
	void drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top);
public:
	SolarisWM(void);
	~SolarisWM(void);
};

#endif