#ifndef WIDGET_LIST_BASE_HH
#define WIDGET_LIST_BASE_HH

#include <Widget/Widget.h>
#include <Widget/Scrollable.h>

namespace Ewok {

class ListBase: public Scrollable {
protected:
	uint32_t itemNum;
	uint32_t itemMargin;
	int32_t  itemStart;
	int32_t  itemSelected;
	/* sub-item scroll offset in pixels along the scroll axis, always kept in
	 * [0, unit). Together with itemStart it gives a pixel-accurate scroll
	 * position (itemStart*unit + pixelOffset) so wheel/drag scrolling is
	 * smooth instead of snapping to whole items. */
	int32_t  pixelOffset;
	/* pixels scrolled per wheel notch; 0 => derive from the item unit */
	int32_t  wheelStep;

	virtual void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) = 0;

	virtual void drawBG(graph_t* g, XTheme* theme, const grect_t& r);
	virtual void onSelect(int sel);
	virtual void onUnselect(int sel);
	virtual void onEnter(int sel);

	virtual void selectByMouse(xevent_t* ev);
	virtual void enterByMouse(xevent_t* ev);

	/* scroll unit in pixels along the scroll axis (item height/width); used to
	 * derive a default wheel step. Subclasses override. */
	virtual int32_t getScrollUnit();

	bool onMouse(xevent_t* ev);
public:
	ListBase();
	~ListBase(void);

	void setItemNum(uint32_t itemNum);
	virtual void select(int sel);
	void enter(int sel);

	void setItemMargin(uint32_t margin);
	void setWheelStep(int32_t step);

	inline uint32_t getItemNum() { return itemNum; }
	inline int32_t getSelected() { return itemSelected; }
};

}

#endif
