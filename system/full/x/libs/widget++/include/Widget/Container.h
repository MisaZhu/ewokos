#ifndef WIDGET_CONTAINER_HH
#define WIDGET_CONTAINER_HH

#include <Widget/Blank.h>
namespace Ewok {

class Container: public Blank {
	Widget* children;
	Widget* childrenEnd;
	uint32_t num;
	int type;

	void layoutV();
	void layoutH();

protected:
	void onResize();
	void onLayout();
	bool onEvent(xevent_t* ev);
	void repaint(graph_t* g, XTheme* theme);
	void onTimer(uint32_t timerFPS);

public:
	static const int FIXED = 0;
	static const int VERTICLE = 1;
	static const int HORIZONTAL = 2;

	void layout();
	void add(Widget* child);
	Widget* get(uint32_t id);
	Widget* get(const string& name);
	void setType(int type);
	void clear();

	Container();
	virtual ~Container();
};

}

#endif