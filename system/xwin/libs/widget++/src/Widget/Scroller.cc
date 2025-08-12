#include <Widget/Scroller.h>
#include <Widget/Scrollable.h>
#include <x++/XTheme.h>

namespace Ewok {

void Scroller::drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
	graph_fill_3d(g, r.x, r.y, r.w, r.h, 
		graph_get_dark_color(theme->basic.bgColor),
		true);
}

void Scroller::drawPos(graph_t* g, XTheme* theme, const grect_t& r) {
	graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, false);
}

void Scroller::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	drawBG(g, theme, r);
	grect_t pos_r;
	if(horizontal) {
		pos_r.h = r.h-2;
		pos_r.y = r.y+1;

		pos_r.w = r.w * ((float)scrollW/range);
		if(pos_r.w < 4)
			pos_r.w = 4;

		pos_r.x = r.x + r.w * ((float)pos/range);
		if(pos_r.x >= (r.x+r.w-pos_r.w))
			pos_r.x = r.x+r.w-pos_r.w-1;
		if(pos_r.x < 0)
			pos_r.x = 0;
	}
	else {
		pos_r.w = r.w-2;
		pos_r.x = r.x+1;

		pos_r.h = r.h * ((float)scrollW/range);
		if(pos_r.h < 4)
			pos_r.h = 4;

		pos_r.y = r.y + r.h * ((float)pos/range);
		if(pos_r.y >= (r.y+r.h-pos_r.h))
			pos_r.y = r.y+r.h-pos_r.h-1;
		if(pos_r.y < 0)
			pos_r.y = 0;
	}

	drawPos(g, theme, pos_r);
}

Scroller::Scroller(bool h) {
	range = 100;
	pos = 0;
	scrollW = range;
	horizontal = h;
	widget = NULL;
	visible = false;
}

void Scroller::setScrollable(Scrollable* widget) {
	this->widget = widget;
}

void Scroller::setScrollW(uint32_t w) {
	if(w > range || w == 0)
		w = range;
	scrollW = w;
	update();

	if(range <= scrollW)
		hide();
	else
		show();
}

void Scroller::setRange(uint32_t range) {
	if(range == 0)
		range = 100;

	if(scrollW > range || scrollW == 0)
		scrollW = range;
	if(pos >= range)
		pos = range - 1;

	this->range = range;
	update();

	if(range <= scrollW) 
		hide();
	else
		show();
}

void Scroller::setPos(uint32_t pos) {
	if(pos >= range)
		pos = range - 1;
	this->pos = pos;
	update();
}

}