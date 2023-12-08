#ifndef WIDGET_WIN_HH
#define WIDGET_WIN_HH

#include <x++/XWin.h>
#include <Widget/RootWidget.h>

namespace Ewok {

class WidgetWin: public XWin {
protected:
	RootWidget rootWidget;
	void onRepaint(graph_t* g);
	void onResize(void);
	void onEvent(xevent_t* ev);

public:
	inline WidgetWin(void) { rootWidget.setWin(this); }
	inline RootWidget* getRootWidget() { return &rootWidget; }
};

}

#endif
