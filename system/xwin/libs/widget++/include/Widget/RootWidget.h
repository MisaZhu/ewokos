#ifndef ROOT_WIDGET_HH
#define ROOT_WIDGET_HH

#include <x++/XWin.h>
#include <Widget/Container.h>

namespace Ewok {

class WidgetWin;
class RootWidget: public Container {
	WidgetWin* xwin;
	bool doRefresh;
	Widget* focusedWidget;
	//void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
public:
	friend WidgetWin;
	RootWidget();

	RootWidget* getRoot(void);
	inline WidgetWin* getWin() { return xwin; }
	inline void setWin(WidgetWin* xwin) { this->xwin = xwin; }
	inline void refresh() { doRefresh = true; }

	void focus(Widget* wd);
	void onFocus();
	inline Widget* getFocused() { return focusedWidget; }

	void repaintWin();
	void update();
	bool sendEvent(xevent_t* ev);
};

}

#endif