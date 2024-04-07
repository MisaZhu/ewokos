#ifndef WIDGET_WIN_HH
#define WIDGET_WIN_HH

#include <x++/XWin.h>
#include <Widget/RootWidget.h>

namespace Ewok {

class WidgetWin: public XWin {
	bool painting;
protected:
	uint32_t timerFPS;
	RootWidget* root;
	uint32_t timerID;
	void onRepaint(graph_t* g);
	void onResize(void);
	void onEvent(xevent_t* ev);
	bool onClose();
public:
	WidgetWin(void);
	~WidgetWin(void);
	inline RootWidget* getRoot() { return root; }

	void setRoot(RootWidget* root);
	void setTimer(uint32_t fps);
	void timerTask();
};

}

#endif
