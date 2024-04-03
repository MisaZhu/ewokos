#include <Widget/Scrollable.h>
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
	if(onScroll(step, horizontal))
		update();
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

}