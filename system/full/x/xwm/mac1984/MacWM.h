#ifndef MACWM_H
#define MACWM_H

#include <stdio.h>
#include <x++/XWM.h>
#include <font/font.h>

using namespace Ewok;

class MacWM : public XWM {
	void drawTitlePattern(graph_t* g, int x, int y, int w, int h, uint32_t fg);
protected:
	void getClose(xinfo_t* info, grect_t* r);
	void getMin(xinfo_t* info, grect_t* r);
	void getMax(xinfo_t* info, grect_t* r);
	void getTitle(xinfo_t* info, grect_t* r);

	void drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top);
public:
	MacWM(void);
	~MacWM(void);
};

#endif