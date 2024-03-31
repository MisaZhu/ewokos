#ifndef WIDGET_LIST_HH
#define WIDGET_LIST_HH

#include <Widget/ListBase.h>

namespace Ewok {

class List: public ListBase {
protected:
	uint32_t itemNumInView;
	uint32_t itemSize;
	bool     fixedItemSize;
	bool     horizontal;

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	void onResize();
	bool onMouse(xevent_t* ev);
	bool onKey(xevent_t* ev);

	virtual void onScroll(int step);
public:
	List();
	~List(void);

	void setHorizontal(bool h);
	void setItemSize(uint32_t itemSize);
	void setItemNumInView(uint32_t itemSize);
	inline uint32_t getItemSize() { return itemSize; }
};

}

#endif
