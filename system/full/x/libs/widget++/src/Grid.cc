#include <Widget/Grid.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/keydef.h>

namespace Ewok {

void Grid::drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
	graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
}

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

		ir.x = r.x + x;
		ir.y = r.y + y;
		ir.w = iw;
		ir.h = itemH;

		graph_set_clip(g, ir.x, ir.y, ir.w, ir.h);
		drawItem(g, theme, i+itemStart, ir);
	}
}

Grid::Grid() {
	itemNum = 0;
	itemStart = 0;
	itemSelected = -1;
	itemW = 32;
	itemH = 32;
	cols = 1;
	rows = 1;
	last_mouse_down = 0;
}

Grid::~Grid(void) {
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
}

void Grid::scroll(int step) {
	itemStart -= (step*cols);
	if(itemStart < 0)
		itemStart = 0;
	else if(itemStart >= itemNum) {
		if((itemNum % cols) == 0)
			itemStart = itemNum - cols;
		else
			itemStart = itemNum - (itemNum%cols);
	}
	update();
}

void Grid::select(int sel) {
	if(sel < 0 || sel >= itemNum || itemSelected == sel)
		return;
	itemSelected = sel;
	onSelect(sel);
	update();
}

void Grid::enter(int sel) {
	if(sel < 0 || sel >= itemNum)
		return;
	onEnter(sel);
}

bool Grid::onMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	uint32_t iw = (uint32_t)(area.w / (float)cols);
	if(ev->state == XEVT_MOUSE_DOWN) {
		last_mouse_down = ipos.y;
		int x = ipos.x / iw;
		int y = ipos.y / itemH;
		select(itemStart + y*cols + x);
		update();
	}
	else if(ev->state == XEVT_MOUSE_DRAG) {
		int pos = ipos.y;
		int mv = (pos - last_mouse_down) / (int)itemH;
		if(abs_32(mv) > 0) {
			last_mouse_down = pos;
			scroll(mv);
		}
	}
	else if(ev->state == XEVT_MOUSE_MOVE) {
		if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP)
			scroll(-1);
		else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN)
			scroll(1);
	}
	else if(ev->state == XEVT_MOUSE_CLICK) {
		int x = ipos.x / iw;
		int y = ipos.y / itemH;
		enter(itemStart + y*cols + x);
	}
	return true;
}

bool Grid::onKey(xevent_t* ev) {
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
			select(sel);
		}
		else if(ev->value.im.value == KEY_ENTER ||
				ev->value.im.value == KEY_BUTTON_START ||
				ev->value.im.value == KEY_BUTTON_A) {
			enter(itemSelected);
		}
	}
	return true;
}

void Grid::onSelect(int32_t index) {
}

void Grid::onEnter(int32_t index) {
}

void Grid::setItemNum(uint32_t num) {
	itemNum = num;
	onResize();
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