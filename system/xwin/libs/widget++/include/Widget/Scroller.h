#ifndef WIDGET_SCROLLER_HH
#define WIDGET_SCROLLER_HH

#include <Widget/Widget.h>

namespace Ewok {

class Scrollable;

class Scroller: public Widget {
	Scrollable* widget;
protected:
	uint32_t range;
	uint32_t pos;
	uint32_t scrollW;
	bool horizontal;
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);

	virtual void drawBG(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void drawPos(graph_t* g, XTheme* theme, const grect_t& r);
public:
	Scroller(bool h = false);

	void setScrollable(Scrollable* widget);
	void setRange(uint32_t range);
	void setPos(uint32_t pos);
	void setScrollW(uint32_t w);
	uint32_t getPos() { return pos; }
};

}

#endif
