#ifndef ROOT_WIDGET_HH
#define ROOT_WIDGET_HH

#include <x++/XWin.h>
#include <Widget/Container.h>

namespace Ewok {

class WidgetWin;
class RootWidget: public Container {
	XWin* xwin;
	bool doRefresh;
	Widget* focusedWidget;
	//void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
public:
	friend WidgetWin;
	RootWidget();
	inline XWin* getWin() { return xwin; }
	inline void setWin(XWin* xwin) { this->xwin = xwin; }
	inline void refresh() { doRefresh = true; }

	void focus(Widget* wd);
	inline Widget* getFocused() { return focusedWidget; }

	void repaintWin();
	void update();
	bool sendEvent(xevent_t* ev);
};

}

#endif