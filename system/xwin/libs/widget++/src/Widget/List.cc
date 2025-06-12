#include <Widget/List.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/keydef.h>

namespace Ewok {

void List::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	drawBG(g, theme, r);
	uint32_t num = 0;
	if(itemNum > itemStart)
		num = itemNum - itemStart;

	if(num > itemNumInView)	{
		num = itemNumInView;
		if(defaultScrollType  == SCROLL_TYPE_H) {
			if((num*itemSize) < r.w)
				num++;
		}
		else {
			if((num*itemSize) < r.h)
				num++;
		}
		if(num > (itemNum - itemStart))
			num = itemNum - itemStart;
	}

	if(defaultScrollType == SCROLL_TYPE_H) {
		for(uint32_t i=0; i<num; i++) {
			grect_t ir;
			ir.x = r.x + i*itemSize + itemMargin;
			ir.y = r.y + itemMargin;
			ir.w = itemSize - itemMargin*2;
			ir.h = r.h - itemMargin*2;
			
			grect_t irc = {ir.x, ir.y, ir.w, ir.h};

			grect_insect(&r, &irc);
			graph_set_clip(g, irc.x, irc.y, irc.w, irc.h);
			drawItem(g, theme, i+itemStart, ir);
		}
	}
	else {
		for(uint32_t i=0; i<num; i++) {
			grect_t ir;
			ir.x = r.x + itemMargin;
			ir.y = r.y + i*itemSize + itemMargin;
			ir.w = r.w - itemMargin*2;
			ir.h = itemSize - itemMargin*2;

			grect_insect(&r, &ir);
			graph_set_clip(g, ir.x, ir.y, ir.w, ir.h);
			drawItem(g, theme, i+itemStart, ir);
		}
	}
}

List::List() {
	itemNumInView = 2;
	itemSize = 0;
	fixedItemSize = false;	
	last_mouse_down = 0;
}

List::~List(void) {
}

void List::updateScroller() {
	if(itemNum <= itemNumInView && itemStart > 0)
		setScrollerInfo(itemNumInView+itemStart, itemStart, itemNumInView, defaultScrollType == SCROLL_TYPE_H);
	else
		setScrollerInfo(itemNum, itemStart, itemNumInView, defaultScrollType  == SCROLL_TYPE_H);
}

void List::onResize() {
	if(fixedItemSize) {
		setItemSize(itemSize);
	}
	else {
		setItemNumInView(itemNumInView);
	}
	updateScroller();
}

bool List::onScroll(int step, bool horizontal) {
	int oldStart = itemStart;
	itemStart -= step;
	if(step < 0) {
		if((itemStart+itemNumInView) >= itemNum) {
			itemStart = itemNum-(int)itemNumInView;
			if((itemNum % itemNumInView) != 0)
				itemStart++;
		}
	}

	if(itemStart < 0)
		itemStart = 0;

	if(oldStart == itemStart)
		return false;

	updateScroller();
	return true;
}

void List::selectByMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	if(itemSize == 0)
		return;

	if(ev->state == MOUSE_STATE_DOWN) {
		int sel = ipos.y / itemSize + itemStart;
		if(defaultScrollType  == SCROLL_TYPE_H)
			sel = ipos.x / itemSize + itemStart; 
		select(sel);
	}
}

void List::enterByMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	if(itemSize == 0)
		return;

	if(ev->state == MOUSE_STATE_CLICK) {
		int sel = ipos.y / itemSize + itemStart;
		if(defaultScrollType  == SCROLL_TYPE_H)
			sel = ipos.x / itemSize + itemStart; 
		enter(sel);
	}
}

bool List::onIM(xevent_t* ev) {
	if(ev->state == XIM_STATE_PRESS) {
		int32_t sel = itemSelected;
		if(ev->value.im.value == KEY_UP ||
				ev->value.im.value == KEY_LEFT) {
			sel--;
			if(sel < 0)
				sel = 0;
			if(sel < itemStart)
				itemStart = sel;

			updateScroller();
			select(sel);
			return true;
		}
		else if(ev->value.im.value == KEY_DOWN ||
				ev->value.im.value == KEY_RIGHT) {
			sel++;
			if(sel >= itemNum)
				sel = itemNum-1;
			if(sel >= itemStart+itemNumInView) {
				itemStart = sel-itemNumInView+1;
				if(itemStart < 0)
					itemStart = 0;
			}
			updateScroller();
			select(sel);
			return true;
		}
	}
	else if(ev->state == XIM_STATE_RELEASE) {
		if(ev->value.im.value == KEY_ENTER ||
				ev->value.im.value == JOYSTICK_START ||
				ev->value.im.value == JOYSTICK_A) {
			enter(itemSelected);
			return true;
		}
	}
	return false;
}

void List::setItemNumInView(uint32_t num) {
	if(num == 0)
		return;

	itemNumInView = num;
	uint32_t size = area.h;
	if(defaultScrollType  == SCROLL_TYPE_H)
		size = area.w;

	itemSize = (float)size / (float)itemNumInView;
	fixedItemSize = false;	
	update();
}

void List::setItemSize(uint32_t size) {
	if(size == 0)
		return;
	itemSize = size;

	size = area.h;
	if(defaultScrollType  == SCROLL_TYPE_H)
		size = area.w;

	itemNumInView = size / itemSize;
	fixedItemSize = true;	
	update();
}

}