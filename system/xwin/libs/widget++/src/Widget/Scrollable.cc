#include <Widget/Scrollable.h>
#include <ewoksys/basic_math.h>
#include <x++/XTheme.h>

namespace Ewok {

void Scrollable::setScrollerV(Scroller *r) {
	scrollerV = r;
	scrollerV->setScrollable(this);
}

void Scrollable::setScrollerH(Scroller *r) {
	scrollerH = r;
	scrollerH->setScrollable(this);
}

void Scrollable::scroll(int step, bool horizontal) {
	if(onScroll(step, horizontal)) {
		updateScroller();
		update();
	}
}

void Scrollable::onResize() {
	updateScroller();
}

void Scrollable::setScrollerInfo(uint32_t range, uint32_t pos, uint32_t scrollW, bool horizontal) {
	if(horizontal) {
		if(scrollerH == NULL)
			return;
		scrollerH->setRange(range);
		scrollerH->setPos(pos);
		scrollerH->setScrollW(scrollW);
	}
	else {
		if(scrollerV == NULL)
			return;
		scrollerV->setRange(range);
		scrollerV->setPos(pos);
		scrollerV->setScrollW(scrollW);
	}
}

Scrollable::Scrollable() {
	scrollerV = NULL;
	scrollerH = NULL;
	defaultScrollType = SCROLL_TYPE_V;
	dragStep = 8;
}

bool Scrollable::onMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	if(ev->state == MOUSE_STATE_DOWN) {
		last_mouse_down.x = ipos.x;
		last_mouse_down.y = ipos.y;
		return true;
	}
	else if(ev->state == MOUSE_STATE_DRAG) {
		int dx = last_mouse_down.x - ipos.x;
		int dy =  last_mouse_down.y - ipos.y;

		if(abs_32(dx) > dragStep) {
			last_mouse_down.x = ipos.x;
			if(dx > 0)
				scroll(-1, true);
			else
				scroll(1, true);
		}
		
		if(abs_32(dy) > dragStep) {
			last_mouse_down.y = ipos.y;
			if(dy > 0)
				scroll(-1, false);
			else
				scroll(1, false);
		}
		return true;
	}
	return false;
}

void Scrollable::setDefaultScrollType(uint8_t type) {
	defaultScrollType = type;
	onResize();
}

void Scrollable::setDragStep(uint16_t step) {
	dragStep = step;	
}

}