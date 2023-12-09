#ifndef ROOT_WIDGET_HH
#define ROOT_WIDGET_HH

#include <x++/XWin.h>
#include <Widget/Container.h>

namespace Ewok {

class WidgetWin;
class RootWidget: public Container {
	XWin* xwin;
public:
	friend WidgetWin;
	XWin* getWin() { return xwin; }
	void updateWin() { xwin->repaint(); }
	void setWin(XWin* win) { xwin = win; }
	void sendEvent(xevent_t* ev) { onEvent(ev); }
};

}

#endif