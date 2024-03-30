#ifndef WIDGET_LIST_HH
#define WIDGET_LIST_HH

#include <Widget/Widget.h>

namespace Ewok {

class List: public Widget {
	int last_mouse_down;
	void scroll(int step);
	void select(int sel);
	void enter(int sel);
protected:
	uint32_t itemNum;
	uint32_t itemNumInView;
	uint32_t itemSize;
	int32_t  itemStart;
	int32_t  itemSelected;
	bool     fixedItemSize;
	bool     horizontal;

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	void onResize();
	bool onMouse(xevent_t* ev);
	bool onKey(xevent_t* ev);

	virtual void drawBG(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) = 0;
	virtual void onSelect(int32_t index);
	virtual void onEnter(int32_t index);
public:
	List();
	~List(void);

	void setItemNum(uint32_t itemNum);
	void setItemSize(uint32_t itemSize);
	void setItemNumInView(uint32_t itemSize);
	void setHorizontal(bool h);

	inline int32_t getSelected() { return itemSelected; }
};

}

#endif
