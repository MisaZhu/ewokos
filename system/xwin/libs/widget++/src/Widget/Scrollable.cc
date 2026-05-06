#include <Widget/Scrollable.h>
#include <Widget/Container.h>
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
		if(scrollerH == NULL) {
			if(scrollerHName.empty())
				return;

			Container* r = getRootContainer();
			Scroller* scrl = (Scroller*)r->getChild(scrollerHName);
			if(scrl == NULL)
				return;
			setScrollerH(scrl);
		}
		scrollerH->setRange(range);
		scrollerH->setPos(pos);
		scrollerH->setScrollW(scrollW);
	}
	else {
		if(scrollerV == NULL) {
			if(scrollerVName.empty())
				return;
			Container* r = getRootContainer();
			Scroller* scrl = (Scroller*)r->getChild(scrollerVName);
			if(scrl == NULL)
				return;
			setScrollerV(scrl);
		}
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
	static bool draging = false;
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);

	if(ev->state == MOUSE_STATE_UP) {
		draging = false;
		return true;
	}

	if(ev->state == MOUSE_STATE_DOWN) {
		last_mouse_down.x = ipos.x;
		last_mouse_down.y = ipos.y;
		if(!draging) {
			draging = true;
			return true;
		}
	}

	if(ev->state == MOUSE_STATE_DRAG || draging) {
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

void Scrollable::setAttr(const string& attr, json_var_t*value) {
	Widget::setAttr(attr, value);
	if(attr == "scrollerV")
		scrollerVName = json_var_get_str(value);
	else if(attr == "scrollerH")
		scrollerHName = json_var_get_str(value);
}


}