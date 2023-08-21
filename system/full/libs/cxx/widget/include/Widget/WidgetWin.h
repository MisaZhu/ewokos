#ifndef WIDGET_WIN_HH
#define WIDGET_WIN_HH

#include <x++/XWin.h>
#include <Widget/Container.h>

namespace Ewok {

class WidgetWin;
class WinWidget: public Container {
	friend WidgetWin;
protected:
	XWin* xwin;
	void onRepaint(graph_t* g, grect_t* rect) {
		(void)g;
		(void)rect;
		//graph_fill(g, rect->x, rect->y, rect->w, rect->h, 0xffffffff);
	}
public:
	void repaintWin(void) {
		if(xwin != NULL)
			xwin->repaint();
	}

	void sendEvent(x_event_t* ev) {
		onEvent(ev);
	}
};

class WidgetWin: public XWin {
protected:
	WinWidget widget;
	void onRepaint(graph_t* g) {
		grect_t r = {0, 0, g->w, g->h};
		widget.repaint(g, &r);
	}

	void onResize(void) override {
		widget.resizeTo(xwin->xinfo->wsr.w, xwin->xinfo->wsr.h);
	}	
public:
	inline WidgetWin(void) {
		widget.xwin = this;
	}
	inline WinWidget* getWidget() { return &widget; }
};

}

#endif
