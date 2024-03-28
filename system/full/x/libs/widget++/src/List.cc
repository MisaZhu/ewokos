#include <Widget/List.h>
#include <ewoksys/basic_math.h>

using namespace EwokSTL;
namespace Ewok {

void List::drawBG(graph_t* g, const Theme* theme, const grect_t& r) {
	graph_fill(g, r.x, r.y, r.w, r.h, theme->bgColor);
}

void List::onRepaint(graph_t* g, const Theme* theme, const grect_t& r) {
	drawBG(g, theme, r);
	uint32_t num = 0;
	if(itemNum > itemStart)
		num = itemNum - itemStart;

	if(num > itemNumInView)	
		num = itemNumInView;

	if(horizontal) {
		for(uint32_t i=0; i<num; i++) {
			grect_t ir;
			ir.x = r.x + i*itemSize;
			ir.y = r.y;
			ir.w = itemSize;
			ir.h = r.h;
			drawItem(g, theme, i+itemStart, ir);
		}
	}
	else {
		for(uint32_t i=0; i<num; i++) {
			grect_t ir;
			ir.x = r.x;
			ir.y = r.y + i*itemSize;
			ir.w = r.w;
			ir.h = itemSize;
			drawItem(g, theme, i+itemStart, ir);
		}
	}
}

List::List() {
	itemNum = 0;
	itemStart = 0;
	itemSelected = -1;
	itemNumInView = 2;
	itemSize = 0;
	last_mouse_down = 0;
	horizontal = false;
	fixedItemSize = false;	
}

List::~List(void) {
}

void List::setHorizontal(bool h) {
	horizontal = h;
	onResize();
}

void List::onResize() {
	if(fixedItemSize) {
		setItemSize(itemSize);
	}
	else {
		setItemNumInView(itemNumInView);
	}
}

bool List::onMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	if(itemSize == 0)
		return true;

	if(ev->state == XEVT_MOUSE_DOWN) {
		if(horizontal)
			last_mouse_down = ipos.x;
		else
			last_mouse_down = ipos.y;
	}
	else if(ev->state == XEVT_MOUSE_DRAG) {
		int pos = ipos.y;
		if(horizontal)
			pos = ipos.x;

		int mv = (pos - last_mouse_down) / (int)itemSize;
		if(abs_32(mv) > 0) {
			last_mouse_down = pos;
			itemStart -= mv;
			if(itemStart < 0)
				itemStart = 0;
			else if(itemStart >= itemNum)
				itemStart = itemNum-1;
			update();
		}
	}
	else if(ev->state == XEVT_MOUSE_CLICK) {
		if(horizontal)
			itemSelected = ipos.x / itemSize + itemStart;
		else
			itemSelected = ipos.y / itemSize + itemStart;
		onSelect(itemSelected);
		update();
	}
	return true;
}

void List::onSelect(int32_t index) {
}

void List::setItemNum(uint32_t num) {
	itemNum = num;
	onResize();
}

void List::setItemNumInView(uint32_t num) {
	if(num == 0)
		return;

	itemNumInView = num;
	uint32_t size = area.h;
	if(horizontal)
		size = area.w;

	itemSize = size / itemNumInView;
	if((size % itemSize) != 0)
		itemNumInView++;

	fixedItemSize = false;	
	update();
}

void List::setItemSize(uint32_t size) {
	if(size == 0)
		return;
	itemSize = size;

	size = area.h;
	if(horizontal)
		size = area.w;

	itemNumInView = size / itemSize;
	if((size % itemSize) != 0)
		itemNumInView++;

	fixedItemSize = true;	
	update();
}

}