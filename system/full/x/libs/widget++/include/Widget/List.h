#ifndef WIDGET_LIST_HH
#define WIDGET_LIST_HH

#include <Widget/ListBase.h>

namespace Ewok {

class List: public ListBase {
protected:
	uint32_t itemNumInView;
	uint32_t itemSize;
	bool     fixedItemSize;
	int      last_mouse_down;

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	void onResize();
	bool onIM(xevent_t* ev);

	void selectByMouse(xevent_t* ev);
	void enterByMouse(xevent_t* ev);

	virtual bool onScroll(int step, bool horizontal);
    virtual void updateScroller();
public:
	List();
	~List(void);

	void setItemSize(uint32_t itemSize);
	void setItemNumInView(uint32_t itemSize);
	inline uint32_t getItemSize() { return itemSize; }
	inline uint32_t getItemNumInView() { return itemNumInView; }
};

}

#endif
