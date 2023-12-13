#ifndef WIDGET_WIN_HH
#define WIDGET_WIN_HH

#include <x++/XWin.h>
#include <Widget/RootWidget.h>

namespace Ewok {

class WidgetWin: public XWin {
protected:
	RootWidget* root;
	void onRepaint(graph_t* g);
	void onResize(void);
	void onEvent(xevent_t* ev);

public:
	inline WidgetWin(void) { root = NULL; }
	inline ~WidgetWin(void) { if(root != NULL) delete root; }
	inline RootWidget* getRoot() { return root; }

	void setRoot(RootWidget* root);
};

}

#endif
