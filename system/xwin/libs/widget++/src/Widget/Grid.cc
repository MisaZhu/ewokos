#include <Widget/Grid.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/keydef.h>

namespace Ewok {

void Grid::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	drawBG(g, theme, r);
	uint32_t num = 0;
	if(itemNum > itemStart)
		num = itemNum - itemStart;

	uint32_t rs = rows;
	if((rows*itemH) < r.h)
		rs++;

	if(num > (rs * cols))	
		num = rs * cols;

	uint32_t iw = (uint32_t)(area.w / (float)cols);

	int x = 0;
	int y = 0;
	grect_t ir;
	for(uint32_t i=0; i<num; i++) {
		if(i != 0) {
			if((i%cols) == 0 && i != 0) {
				y += itemH;
				x = 0;
			}
			else
				x += iw;
		}	

		ir.x = r.x + x + itemMargin;
		ir.y = r.y + y + itemMargin;
		ir.w = iw - itemMargin*2;
		ir.h = itemH - itemMargin*2;

		grect_t irc = {ir.x, ir.y, ir.w, ir.h};

		grect_insect(&r, &irc);
		graph_set_clip(g, irc.x, irc.y, irc.w, irc.h);
		drawItem(g, theme, i+itemStart, ir);
	}
}

Grid::Grid() {
	itemW = 32;
	itemH = 32;
	cols = 1;
	rows = 1;
	last_mouse_down = 0;
}

Grid::~Grid(void) {
}

void Grid::updateScroller() {
	if(scrollerV == NULL)
		return;
	uint32_t itemNumInView = cols * rows;
	if(itemNum <= itemNumInView && itemStart > 0)
		setScrollerInfo(itemNumInView+itemStart, itemStart, itemNumInView, false);
	else
		setScrollerInfo(itemNum, itemStart, itemNumInView, false);
}

void Grid::onResize() {
	cols = area.w / itemW;
	rows = area.h / itemH;

	if(cols == 0) 
		cols = 1;
	if(rows == 0)
		rows = 1;

	if((itemStart % cols) != 0)
		itemStart -= (itemStart %cols);
	updateScroller();
}

bool Grid::onScroll(int step, bool horizontal) {
	int oldStart = itemStart;
	itemStart -= (step*cols);
	if(step < 0) {
		if((itemStart + rows*cols)>= itemNum) {
			uint32_t i = itemNum%cols;
			if(i == 0)
				itemStart = itemNum - rows*cols;
			else
				itemStart = itemNum - i - (rows-1)*cols;
		}
	}

	if(itemStart < 0)
		itemStart = 0;

	if(oldStart == itemStart)
		return false;

	updateScroller();
	return true;
}

void Grid::selectByMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	uint32_t iw = (uint32_t)(area.w / (float)cols);
	if(ev->state == MOUSE_STATE_DOWN) {
		int x = ipos.x / iw;
		int y = ipos.y / itemH;
		int sel = itemStart + y*cols + x;
		select(sel);
	}
}

void Grid::enterByMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	uint32_t iw = (uint32_t)(area.w / (float)cols);
	if(ev->state == MOUSE_STATE_CLICK) {
		int x = ipos.x / iw;
		int y = ipos.y / itemH;
		int sel = itemStart + y*cols + x;
		enter(sel);
	}
}

bool Grid::onIM(xevent_t* ev) {
	if(ev->state == XIM_STATE_PRESS) {
		int32_t sel = itemSelected;
		if(ev->value.im.value == KEY_LEFT ||
				ev->value.im.value == KEY_UP) {
			if(ev->value.im.value == KEY_UP)
				sel -= cols;
			else 
				sel--;

			if(sel < 0)
				sel = 0;
			if(sel < itemStart) {
				itemStart -= cols;
				if(itemStart < 0)
					itemStart = 0;
			}
			updateScroller();
			select(sel);
		}
		else if(ev->value.im.value == KEY_RIGHT ||
				ev->value.im.value == KEY_DOWN) {
			if(ev->value.im.value == KEY_DOWN)
				sel += cols;
			else
				sel++;

			if(sel >= itemNum)
				sel = itemNum-1;
			if(sel >= itemStart+cols*rows) {
				itemStart += cols;
			}
			updateScroller();
			select(sel);
		}
	}
	else if(ev->state == XIM_STATE_RELEASE) {
		if(ev->value.im.value == KEY_ENTER ||
				ev->value.im.value == JOYSTICK_START ||
				ev->value.im.value == JOYSTICK_A) {
			enter(itemSelected);
		}
	}
	return true;
}

void Grid::setItemSize(uint32_t iw, uint32_t ih) {
	if(iw > 0)
		itemW = iw;
	if(ih > 0)
		itemH = ih;
	onResize();
	update();
}

}