#ifndef MACWM_H
#define MACWM_H

#include <stdio.h>
#include <x++/XWM.h>
#include <ttf/ttf.h>

using namespace Ewok;

class MacWM : public XWM {
	uint32_t bgColor;
	uint32_t fgColor;
	uint32_t bgTopColor;
	uint32_t fgTopColor;
	uint32_t desktopFGColor;
	uint32_t desktopBGColor;

	ttf_font_t* font;
	graph_t* bgImg;
	void drawTitlePattern(graph_t* g, int x, int y, int w, int h, uint32_t fg);

protected:
	void getColor(uint32_t *fg, uint32_t* bg, bool top);
	void drawDesktop(graph_t* g);
	void drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top);
public:
	void readConfig(void);
	MacWM(void);
	~MacWM(void);
};

#endif