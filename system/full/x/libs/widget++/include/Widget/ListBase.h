#ifndef WIDGET_LIST_BASE_HH
#define WIDGET_LIST_BASE_HH

#include <Widget/Widget.h>

namespace Ewok {

class ListBase: public Widget {
protected:
	int      last_mouse_down;
	uint32_t itemNum;
	int32_t  itemStart;
	int32_t  itemSelected;

	virtual void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) = 0;

	virtual void drawBG(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void onSelect(int sel);
	virtual void onEnter(int sel);
	virtual void onScroll(int step);
public:
	ListBase();
	~ListBase(void);

	void setItemNum(uint32_t itemNum);
	void select(int sel);
	void enter(int sel);

	inline uint32_t getItemNum() { return itemNum; }
	inline int32_t getSelected() { return itemSelected; }
};

}

#endif
