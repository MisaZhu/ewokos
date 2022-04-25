#ifndef MACWM_H
#define MACWM_H

#include <stdio.h>
#include <x++/XWM.h>
#include <sys/klog.h>
#include <graph/graph.h>

using namespace Ewok;

class MacWM : public XWM {
	bool doWelcome;
	graph_t* img;
	void drawWelcome(graph_t* g);
	void drawTitlePattern(graph_t* g, int x, int y, int w, int h, uint32_t fg);

protected:
	void drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	void drawDesktop(graph_t* g);

public:
	MacWM(void);
};

#endif