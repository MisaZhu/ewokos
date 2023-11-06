#ifndef WIDGET_WIN_HH
#define WIDGET_WIN_HH

#include <x++/XWin.h>
#include <Widget/Container.h>

namespace Ewok {

class WidgetWin;
class WinWidget: public Container {
	friend WidgetWin;
	XWin* xwin;
protected:
	void onRepaint(graph_t* g, grect_t* rect) {
		Container::onRepaint(g, rect);
	}

	XWin* getWin() {
		return xwin;
	}

	void sendEvent(xevent_t* ev) {
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

	void onEvent(xevent_t* ev) {
		widget.sendEvent(ev);
	}
public:
	inline WidgetWin(void) {
		widget.xwin = this;
	}
	inline WinWidget* getWidget() { return &widget; }
};

}

#endif
