#include <Widget/Columns.h>

using namespace Ewok;

void Columns::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.docBGColor);

	font_t* font = theme->getFont();
	if(font == NULL)
		return;
	
	if(rowH == 0) {
		rowH = font_get_height(font, theme->basic.fontSize);
	}

	int32_t y = 0;
	int32_t x = r.x;
	int num = 0;

	for(uint32_t i=0; i<colNum; i++) {
		graph_set_clip(g, x, y-off_y, cols[i].width, rowH);
		graph_fill_3d(g, x, y-off_y, cols[i].width, rowH, theme->basic.widgetBGColor, false);
		graph_draw_text_font(g, x+2, y - off_y, cols[i].title.c_str(),
				font, theme->basic.fontSize, theme->basic.docFGColor);
		x += cols[i].width;
	}
	x = r.x;
	y += rowH;

	for(uint32_t i=0; i<rowNum; i++) {
		grect_t rr = {r.x, y-off_y, r.w, rowH};
		drawRowBG(g, theme, i, r);
		for(uint32_t j=0; j<colNum; j++) {
			grect_t r = {x, y-off_y, cols[j].width, rowH};
			graph_set_clip(g, r.x, r.y, r.w, r.h);
			drawItem(g, theme, i, j, r);
			x += cols[j].width;
		}
		x = r.x;
		y += rowH;
	}

	paintH = y;
	updateScroller();
}

void Columns::setRowSize(uint32_t size) {
	rowH = size;
	layout();
	update();
}

bool Columns::onScroll(int step, bool horizontal) {
	int old_off = off_y;
	off_y -= step*16;

	if(step < 0) {
		if((off_y + area.h) >= paintH)
			off_y = paintH - area.h;
	}

	if(off_y < 0)
		off_y = 0;
	
	if(off_y == old_off)
		return false;

	return true;
}

void Columns::onResize() {
	layout();
}

void Columns::layout() {
	int asize = area.w;
	int anum = 0;
	for(uint32_t i=0; i<colNum; i++) {
		if(cols[i].fixed)
			asize -= cols[i].width;
		else
			anum++;
	}

	if(asize < 0)
		asize = 0;
	else if(anum > 0)
		asize = (float)asize / (float)anum;

	for(uint32_t i=0; i<colNum; i++) {
		if(!cols[i].fixed) {
			cols[i].width = asize;
		}
	}
}

void Columns::updateScroller() {
	if(paintH <= area.h && off_y > 0)
		setScrollerInfo(area.h + off_y, off_y, area.h, false);
	else
		setScrollerInfo(paintH, off_y, area.h, false);
}

bool Columns::onMouse(xevent_t* ev) {
	bool ret = Scrollable::onMouse(ev);

	if(ev->state == MOUSE_STATE_MOVE) {
		if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
			scroll(-1, defaultScrollType == SCROLL_TYPE_H);
			ret = true;
		}
		else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
			scroll(1, defaultScrollType == SCROLL_TYPE_H);
			ret = true;
		}
	}
	return ret;
}

Columns::Columns() {
	colNum = 0;
	rowNum = 0;
	off_y = 0;
	rowH = 0;
	paintH = 0;
}

void Columns::add(const string& title, uint32_t width) {
	if(colNum >= MAX_COLS)
		return;
	cols[colNum].title = title;
	cols[colNum].width = width;
	if(width == 0)
		cols[colNum].fixed = false;
	else 
		cols[colNum].fixed = true;
	colNum++;
	layout();
	update();
}