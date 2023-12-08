#ifndef WIDGET_CONTAINER_HH
#define WIDGET_CONTAINER_HH

#include <Widget/Widget.h>
namespace Ewok {

class Container: public Widget {
	Widget* children;
	Widget* childrenEnd;
	uint32_t num;
	int type;

	void layoutV();
	void layoutH();
	void layout();

protected:
	void onResize();
	bool onEvent(xevent_t* ev);

public:
	static const int FIXED = 0;
	static const int VERTICLE = 1;
	static const int HORIZONTAL = 2;

	void add(Widget* child);
	void repaint(graph_t* g);
	void setType(int type);
	void clear();

	Container();
	virtual ~Container();
};

}

#endif