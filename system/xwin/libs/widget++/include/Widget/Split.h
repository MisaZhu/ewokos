#ifndef WIDGET_SPLIT_HH
#define WIDGET_SPLIT_HH

#include <Widget/Widget.h>

namespace Ewok {

class Split: public Widget {
	void resizeAttach(int d);
	bool moveSplit(xevent_t* ev);
protected:
	bool horizontal;
	uint32_t width;
	uint32_t barSize;
	uint32_t step;
	Widget*  attachedWidget;
	bool  attachedAfter;
	int last_mouse_down;
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);

	bool onMouse(xevent_t* ev);

	void onAdd();
public:
	Split();

	void setWidth(uint32_t w);
	void setBarSize(uint32_t bsize);
	void setStep(uint32_t step);
	void attach(Widget* wd, bool after = false);
};

}

#endif
