#ifndef ROOT_WIDGET_HH
#define ROOT_WIDGET_HH

#include <x++/XWin.h>
#include <Widget/Container.h>

namespace Ewok {

class WidgetWin;
class RootWidget: public Container {
	XWin* xwin;
	bool doRefresh;
public:
	friend WidgetWin;
	inline RootWidget() { doRefresh = false; }
	inline void setWin(XWin* win) { xwin = win; }
	inline XWin* getWin() { return xwin; }
	inline void refresh() { doRefresh = true; }

	void repaintWin();
	void update();
	void sendEvent(xevent_t* ev);

};

}

#endif