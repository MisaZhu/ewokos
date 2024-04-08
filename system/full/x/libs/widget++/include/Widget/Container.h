#ifndef WIDGET_CONTAINER_HH
#define WIDGET_CONTAINER_HH

#include <Widget/Blank.h>
namespace Ewok {

class Container: public Blank {
	Widget* children;
	Widget* childrenEnd;
	uint32_t num;
	uint8_t type;

	void layoutV();
	void layoutH();
	void layoutO();
protected:
	void onResize();
	void onLayout();
	bool onEvent(xevent_t* ev);
	void repaint(graph_t* g, XTheme* theme);
	void onTimer(uint32_t timerFPS);

public:
	static const uint8_t FIXED = 0;
	static const uint8_t VERTICLE = 1;
	static const uint8_t HORIZONTAL = 2;
	static const uint8_t OVERLAP = 3;

	void layout();
	void add(Widget* child);
	Widget* get(uint32_t id);
	Widget* get(const string& name);
	void setType(uint8_t type);
	uint8_t  getType() { return type; }
	void clear();

	Container();
	virtual ~Container();
};

}

#endif