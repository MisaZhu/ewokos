#ifndef WIDGET_WIN_HH
#define WIDGET_WIN_HH

#include <x++/XWin.h>
#include <Widget/RootWidget.h>

namespace Ewok {

class WidgetWin: public XWin {
	bool painting;
protected:
	RootWidget* root;
	Theme* theme;
	uint32_t timerID;
	void onRepaint(graph_t* g);
	void onResize(void);
	void onEvent(xevent_t* ev);

public:
	WidgetWin(void);
	~WidgetWin(void);
	inline RootWidget* getRoot() { return root; }
	inline Theme* getTheme() { return theme; }

	void setRoot(RootWidget* root);
	void setTimer(uint32_t fps);
	void timerTask();
};

}

#endif
