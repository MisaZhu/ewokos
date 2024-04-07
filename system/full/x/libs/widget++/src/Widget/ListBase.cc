#include <Widget/ListBase.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/keydef.h>

namespace Ewok {

ListBase::ListBase() {
	itemNum = 0;
	itemStart = 0;
	itemSelected = -1;
}

ListBase::~ListBase(void) {
}

void ListBase::drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
	if(!isAlpha())
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
}

void ListBase::onSelect(int sel) {
}

void ListBase::onEnter(int sel) {
}


void ListBase::selectByMouse(xevent_t* ev) {
}

void ListBase::enterByMouse(xevent_t* ev) {
}

bool ListBase::onMouse(xevent_t* ev) {
	bool ret = Scrollable::onMouse(ev);

	if(ev->state == XEVT_MOUSE_DOWN) {
		selectByMouse(ev);
		ret = true;
	}
	else if(ev->state == XEVT_MOUSE_MOVE) {
		if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
			scroll(-1, defaultScrollType == SCROLL_TYPE_H);
			ret = true;
		}
		else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
			scroll(1, defaultScrollType == SCROLL_TYPE_H);
			ret = true;
		}
	}
	else if(ev->state == XEVT_MOUSE_CLICK) {
		enterByMouse(ev);
		ret = true;
	}
	return ret;
}

void ListBase::select(int sel) {
	if(sel < 0 || sel >= itemNum || itemSelected == sel)
		return;
	itemSelected = sel;
	onSelect(sel);
	update();
}

void ListBase::enter(int sel) {
	if(sel < 0 || sel >= itemNum)
		return;
	onEnter(sel);
}

void ListBase::setItemNum(uint32_t num) {
	itemNum = num;
	onResize();
}

}