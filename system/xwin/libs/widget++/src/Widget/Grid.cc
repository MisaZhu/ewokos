#include <Widget/Grid.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/keydef.h>

namespace Ewok {

void Grid::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	drawBG(g, theme, r);
	if(itemNum == 0 || cols == 0 || itemH == 0)
		return;

	uint32_t num = 0;
	if(itemNum > (uint32_t)itemStart)
		num = itemNum - itemStart;

	/* rows needed to cover the view plus the part scrolled off the top edge
	 * (pixelOffset) so no gap appears mid smooth-scroll */
	uint32_t needRows = (r.h + pixelOffset + itemH - 1) / itemH;
	uint32_t need = needRows * cols;
	if(num > need)
		num = need;

	uint32_t iw = (uint32_t)(area.w / (float)cols);

	int x = 0;
	int y = 0;
	grect_t ir;
	for(uint32_t i=0; i<num; i++) {
		if(i != 0) {
			if((i%cols) == 0) {
				y += itemH;
				x = 0;
			}
			else
				x += iw;
		}

		ir.x = r.x + x + itemMargin;
		ir.y = r.y + y - pixelOffset + itemMargin;
		ir.w = iw - itemMargin*2;
		ir.h = itemH - itemMargin*2;

		grect_t irc = {ir.x, ir.y, ir.w, ir.h};

		grect_insect(&r, &irc);
		if(irc.w <= 0 || irc.h <= 0)
			continue;
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
	if(cols == 0 || itemH == 0)
		return;
	/* report the scroller in pixel units so the thumb tracks the smooth,
	 * sub-row scroll position instead of snapping to whole rows */
	uint32_t numRows = (itemNum + cols - 1) / cols;
	uint32_t contentLen = numRows * itemH;
	uint32_t pos = (uint32_t)(itemStart / (int32_t)cols) * itemH + (uint32_t)pixelOffset;
	setScrollerInfo(contentLen, pos, area.h, false);
}

int32_t Grid::getScrollUnit() {
	return (int32_t)itemH;
}

bool Grid::onScrollPixels(int dx, int dy) {
	(void)dx;
	if(itemH == 0 || cols == 0)
		return false;
	if(dx == 0 && dy == 0)
		return false;

	int32_t unit = (int32_t)itemH;
	int32_t c = (int32_t)cols;
	int32_t numRows = (int32_t)((itemNum + cols - 1) / cols);
	int32_t contentLen = numRows * unit;
	int32_t maxPos = contentLen - area.h;
	if(maxPos < 0)
		maxPos = 0;

	int32_t row = itemStart / c;
	int32_t pos = row * unit + pixelOffset + dy;
	if(pos < 0)
		pos = 0;
	if(pos > maxPos)
		pos = maxPos;

	int32_t newRow = pos / unit;
	int32_t newOff = pos % unit;
	bool changed = (newRow != row) || (newOff != pixelOffset);
	itemStart = newRow * c;
	pixelOffset = newOff;
	if(changed)
		updateScroller();
	return true;
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
	if(itemH > 0 && pixelOffset >= (int32_t)itemH)
		pixelOffset = 0;
	updateScroller();
}

bool Grid::onScroll(int step, bool horizontal) {
	int oldStart = itemStart;
	pixelOffset = 0;
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
		int y = (ipos.y + pixelOffset) / itemH;
		int sel = itemStart + y*cols + x;
		select(sel);
	}
}

void Grid::enterByMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	uint32_t iw = (uint32_t)(area.w / (float)cols);
	if(ev->state == MOUSE_STATE_CLICK) {
		int x = ipos.x / iw;
		int y = (ipos.y + pixelOffset) / itemH;
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
			pixelOffset = 0;
			updateScroller();
			select(sel);
			return true;
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
			pixelOffset = 0;
			updateScroller();
			select(sel);
			return true;
		}
		else if(ev->value.im.value == KEY_ENTER ||
				ev->value.im.value == JOYSTICK_START ||
				ev->value.im.value == JOYSTICK_A) {
			enter(itemSelected);
			update();
			return true;
		}
	}
	return false;
}

/* scroll the selected item's row into view with pixel accuracy: only the
 * minimum distance is scrolled, so a partially-visible bottom row is aligned
 * flush to the bottom edge instead of jumping a whole row. */
void Grid::select(int sel) {
	if(cols > 0 && itemH > 0 && sel >= 0 && sel < (int32_t)itemNum) {
		int32_t c = (int32_t)cols;
		int32_t unit = (int32_t)itemH;
		int32_t viewLen = area.h;
		int32_t pos = (itemStart / c) * unit + pixelOffset;
		int32_t rowTop = (sel / c) * unit;
		int32_t rowBottom = rowTop + unit;

		if(rowTop < pos)
			pos = rowTop;                     /* align to top edge */
		else if(rowBottom > pos + viewLen)
			pos = rowBottom - viewLen;        /* align to bottom edge */

		int32_t numRows = (int32_t)((itemNum + cols - 1) / cols);
		int32_t maxPos = numRows * unit - viewLen;
		if(maxPos < 0)
			maxPos = 0;
		if(pos < 0)
			pos = 0;
		if(pos > maxPos)
			pos = maxPos;

		itemStart = (pos / unit) * c;
		pixelOffset = pos % unit;
		updateScroller();
	}
	ListBase::select(sel);
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