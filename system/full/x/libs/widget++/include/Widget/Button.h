#ifndef WIDGET_BUTTON_HH
#define WIDGET_BUTTON_HH

#include <Widget/Widget.h>

namespace Ewok {

class Button: public Widget {
protected:
	uint8_t state;
	virtual void paintDown(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void paintUp(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void paintDisabled(graph_t* g, XTheme* theme, const grect_t& r);
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	virtual bool onMouse(xevent_t* ev);

public:
	enum {
		STATE_UP = 0,
		STATE_DOWN
	};

	Button();
};

}

#endif
