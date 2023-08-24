#ifndef MACWM_H
#define MACWM_H

#include <stdio.h>
#include <x++/XWM.h>
#include <font/font.h>

using namespace Ewok;

class SolarisWM : public XWM {
	uint32_t bgColor;
	uint32_t fgColor;
	uint32_t bgTopColor;
	uint32_t fgTopColor;
	uint32_t frameW;
	uint32_t desktopFGColor;
	uint32_t desktopBGColor;

	font_t font;
	graph_t* bgImg;

protected:
	void drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawFrame(graph_t* g, xinfo_t* info, bool top);
	void getColor(uint32_t *fg, uint32_t* bg, bool top);
	void drawDesktop(graph_t* g);
	void drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top);
public:
	void readConfig(void);
	SolarisWM(void);
	~SolarisWM(void);
};

#endif