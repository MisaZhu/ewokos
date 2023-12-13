#ifndef ROOT_WIDGET_HH
#define ROOT_WIDGET_HH

#include <x++/XWin.h>
#include <Widget/Container.h>
#include <Widget/Theme.h>

namespace Ewok {

class WidgetWin;
class RootWidget: public Container {
	XWin* xwin;
	bool doRefresh;
	Theme* theme;
	Widget* focusedWidget;
	//void onRepaint(graph_t* g, const Theme* theme, const grect_t& r);
public:
	friend WidgetWin;
	RootWidget();
	inline XWin* getWin() { return xwin; }
	inline void setWin(XWin* xwin) { this->xwin = xwin; }
	inline void refresh() { doRefresh = true; }
	inline void setFocus(Widget* wd) { focusedWidget = wd; }

	void repaintWin();
	void update();
	void sendEvent(xevent_t* ev);
	void loadTheme(const char* theme);
};

}

#endif