#include <Widget/List.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/keydef.h>

namespace Ewok {

void List::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	drawBG(g, theme, r);
	if(itemNum == 0 || itemSize == 0)
		return;

	uint32_t num = 0;
	if(itemNum > (uint32_t)itemStart)
		num = itemNum - itemStart;

	/* number of items needed to cover the view plus the part scrolled off the
	 * leading edge (pixelOffset), so no gap appears while smooth-scrolling */
	uint32_t viewLen = (defaultScrollType == SCROLL_TYPE_H) ? r.w : r.h;
	uint32_t need = (viewLen + pixelOffset + itemSize - 1) / itemSize;
	if(num > need)
		num = need;

	if(defaultScrollType == SCROLL_TYPE_H) {
		for(uint32_t i=0; i<num; i++) {
			grect_t ir;
			ir.x = r.x + (int32_t)(i*itemSize) - pixelOffset + itemMargin;
			ir.y = r.y + itemMargin;
			ir.w = itemSize - itemMargin*2;
			ir.h = r.h - itemMargin*2;

			grect_t irc = {ir.x, ir.y, ir.w, ir.h};

			grect_insect(&r, &irc);
			if(irc.w <= 0 || irc.h <= 0)
				continue;
			graph_set_clip(g, irc.x, irc.y, irc.w, irc.h);
			drawItem(g, theme, i+itemStart, ir);
		}
	}
	else {
		for(uint32_t i=0; i<num; i++) {
			grect_t ir;
			ir.x = r.x + itemMargin;
			ir.y = r.y + (int32_t)(i*itemSize) - pixelOffset + itemMargin;
			ir.w = r.w - itemMargin*2;
			ir.h = itemSize - itemMargin*2;

			grect_t irc = {ir.x, ir.y, ir.w, ir.h};

			grect_insect(&r, &irc);
			if(irc.w <= 0 || irc.h <= 0)
				continue;
			graph_set_clip(g, irc.x, irc.y, irc.w, irc.h);
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
	bool horiz = (defaultScrollType == SCROLL_TYPE_H);
	if(itemSize == 0)
		return;
	/* report the scroller in pixel units so the thumb tracks the smooth,
	 * sub-item scroll position instead of snapping to whole entries */
	uint32_t viewLen = horiz ? area.w : area.h;
	uint32_t contentLen = itemNum * itemSize;
	uint32_t pos = (uint32_t)itemStart * itemSize + (uint32_t)pixelOffset;
	setScrollerInfo(contentLen, pos, viewLen, horiz);
}

int32_t List::getScrollUnit() {
	return (int32_t)itemSize;
}

bool List::onScrollPixels(int dx, int dy) {
	if(itemSize == 0)
		return false;
	bool horiz = (defaultScrollType == SCROLL_TYPE_H);
	int32_t d = horiz ? dx : dy;
	/* consume the gesture on our axis even when clamped at an end, so the
	 * legacy item-step path in Scrollable does not also fire */
	if(dx == 0 && dy == 0)
		return false;

	int32_t unit = (int32_t)itemSize;
	int32_t viewLen = horiz ? area.w : area.h;
	int32_t contentLen = (int32_t)itemNum * unit;
	int32_t maxPos = contentLen - viewLen;
	if(maxPos < 0)
		maxPos = 0;

	int32_t pos = itemStart * unit + pixelOffset + d;
	if(pos < 0)
		pos = 0;
	if(pos > maxPos)
		pos = maxPos;

	int32_t newStart = pos / unit;
	int32_t newOff = pos % unit;
	bool changed = (newStart != itemStart) || (newOff != pixelOffset);
	itemStart = newStart;
	pixelOffset = newOff;
	if(changed)
		updateScroller();
	return true;
}

void List::onResize() {
	if(fixedItemSize) {
		setItemSize(itemSize);
	}
	else {
		setItemNumInView(itemNumInView);
	}
	if(itemSize > 0 && pixelOffset >= (int32_t)itemSize)
		pixelOffset = 0;
	updateScroller();
}

bool List::onScroll(int step, bool horizontal) {
	int oldStart = itemStart;
	pixelOffset = 0;
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

/* scroll the selected item into view with pixel accuracy: only the minimum
 * distance is scrolled, so a partially-visible item at the trailing edge is
 * aligned flush to that edge instead of jumping a whole entry. */
void List::select(int sel) {
	if(itemSize > 0 && sel >= 0 && sel < (int32_t)itemNum) {
		bool horiz = (defaultScrollType == SCROLL_TYPE_H);
		int32_t unit = (int32_t)itemSize;
		int32_t viewLen = horiz ? area.w : area.h;
		int32_t pos = itemStart * unit + pixelOffset;
		int32_t itemTop = sel * unit;
		int32_t itemBottom = itemTop + unit;

		if(itemTop < pos)
			pos = itemTop;                    /* align to leading edge */
		else if(itemBottom > pos + viewLen)
			pos = itemBottom - viewLen;       /* align to trailing edge */

		int32_t maxPos = (int32_t)itemNum * unit - viewLen;
		if(maxPos < 0)
			maxPos = 0;
		if(pos < 0)
			pos = 0;
		if(pos > maxPos)
			pos = maxPos;

		itemStart = pos / unit;
		pixelOffset = pos % unit;
		updateScroller();
	}
	ListBase::select(sel);
}

void List::selectByMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	if(itemSize == 0)
		return;

	if(ev->state == MOUSE_STATE_DOWN) {
		int sel = (ipos.y + pixelOffset) / itemSize + itemStart;
		if(defaultScrollType  == SCROLL_TYPE_H)
			sel = (ipos.x + pixelOffset) / itemSize + itemStart; 
		select(sel);
	}
}

void List::enterByMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	if(itemSize == 0)
		return;

	if(ev->state == MOUSE_STATE_CLICK) {
		int sel = (ipos.y + pixelOffset) / itemSize + itemStart;
		if(defaultScrollType  == SCROLL_TYPE_H)
			sel = (ipos.x + pixelOffset) / itemSize + itemStart; 
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

			pixelOffset = 0;
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
			pixelOffset = 0;
			updateScroller();
			select(sel);
			return true;
		}
		else if(ev->value.im.value == KEY_ENTER ||
				ev->value.im.value == JOYSTICK_START ||
				ev->value.im.value == JOYSTICK_A) {
			/* 按下沿触发 (与方向键一致)。曾在 RELEASE 沿触发: 释放沿依赖
			 * "按键从快照消失"的差分, 中间一拍状态未被采样到时释放沿丢失,
			 * 启动被压到下一个按键事件才带出 (Enter 无响应, 光标键一按才
			 * 响应)。按下沿即启动, 不依赖释放沿的送达。 */
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