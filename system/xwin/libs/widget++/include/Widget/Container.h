#ifndef WIDGET_CONTAINER_HH
#define WIDGET_CONTAINER_HH

#include <Widget/Blank.h>
namespace Ewok {

class Container: public Blank {
	void layoutV();
	void layoutH();
	void layoutO();
protected:
	Widget* children;
	Widget* childrenEnd;
	uint32_t num;
	uint8_t type;

	void onResize();
	void onLayout();
	bool onEvent(xevent_t* ev);
	void repaint(graph_t* g, XTheme* theme);
	void timerTrigger(uint32_t timerFPS, uint32_t timerStep);
	void doTimer();
	void setAttr(const string& attr, json_var_t* value);
public:
	static const uint8_t FIXED = 0;
	static const uint8_t VERTICLE = 1;
	static const uint8_t HORIZONTAL = 2;
	static const uint8_t OVERLAP = 3;

	void layout();
	void add(Widget* child);
	Widget* getChild(uint32_t id);
	Widget* getChild(const string& name);
	bool has(Widget* wd);
	void setType(uint8_t type);
	uint8_t  getType() { return type; }
	void clear();

	Container();
	virtual ~Container();
};

}

#endif