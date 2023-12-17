#ifndef WIDGET_WIN_HH
#define WIDGET_WIN_HH

#include <x++/XWin.h>
#include <Widget/RootWidget.h>

namespace Ewok {

class WidgetWin: public XWin {
protected:
	RootWidget* root;
	Theme* theme;
	void onRepaint(graph_t* g);
	void onResize(void);
	void onEvent(xevent_t* ev);

public:
	WidgetWin(void);
	~WidgetWin(void);
	inline RootWidget* getRoot() { return root; }
	inline Theme* getTheme() { return theme; }

	void setRoot(RootWidget* root);
};

}

#endif
