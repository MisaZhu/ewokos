#ifndef MACWM_H
#define MACWM_H

#include <stdio.h>
#include <x++/XWM.h>

using namespace Ewok;

class MacWM : public XWM {
	void drawTitlePattern(graph_t* g, int x, int y, int w, int h, uint32_t fg);

protected:
	void drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top);
public:
	MacWM(void);
};

#endif