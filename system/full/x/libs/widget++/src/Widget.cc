#include <Widget/Widget.h>
#include <Widget/RootWidget.h>

namespace Ewok {

void Widget::onRepaint(graph_t* g) {
	if(bgColor == 0x0)
		return;
	grect_t rect = getRootArea();
	graph_fill(g, rect.x, rect.y, rect.w, rect.h, bgColor);
}

bool Widget::onEvent(xevent_t* ev) { 
	if(ev->type == XEVT_MOUSE) {
		grect_t r = getScreenArea();
		if(ev->value.mouse.x > r.x && ev->value.mouse.x < (r.x+r.w) &&
				ev->value.mouse.y > r.y && ev->value.mouse.y < (r.y+r.h))
			return onMouse(ev);
	}
	else if(ev->type == XEVT_IM) {
			return onKey(ev);
	}
	return false; 
}

RootWidget* Widget::getRoot(void) {
	if(father == NULL)
		return (RootWidget*)this;

	Widget* wd = father;
	while(wd != NULL && wd->father != NULL)
		wd = wd->father;
	return (RootWidget*)wd;
}

void Widget::repaint(graph_t* g) {
	onRepaint(g);
	dirty = false;
}

void Widget::update() {
	dirty = true;
	if(isAlpha() && father != NULL) {
		father->update();
	}
	else {
		RootWidget* root = getRoot();
		if(root != NULL)
			root->updateWin();
	}
}

gsize_t Widget::getMinSize(void) {
	gsize_t sz = { marginH*2, marginV*2 };
	return sz;
}

void Widget::fixedMinSize(void) {
	gsize_t sz = getMinSize();
	resizeTo(sz.w, sz.h);
	setFixed(true);
}

void Widget::resizeTo(int w, int h) {
	if(area.w == w && area.h == h)
		return;
	area.w = w;
	area.h = h;
	onResize();
}

void Widget::resize(int dw, int dh) {
	resizeTo(area.w + dw, area.h + dh);
}

void Widget::moveTo(int x, int y) {
	if(area.x == x && area.y == y)
		return;
	area.x = x;
	area.y = y;
	onMove();
}

void Widget::move(int dx, int dy) {
	moveTo(area.x + dx, area.y + dy);
}

void Widget::setArea(int x, int y, int w, int h) {
	resizeTo(w, h);
	moveTo(x, y);
}

gpos_t Widget::getRootPos(int32_t x, int32_t y) {
	gpos_t pos = {area.x+x, area.y+y};
	Widget* wd = father;
	while(wd != NULL) {
		pos.x += wd->area.x;
		pos.y += wd->area.y;
		wd = wd->father;
	}
	return pos;
}

gpos_t Widget::getScreenPos(int32_t x, int32_t y) {
	gpos_t pos = getRootPos(x, y);
	RootWidget*  root = getRoot();
	if(root == NULL)
		return pos;

	XWin* win = root->getWin();
	if(win == NULL)
		return pos;

	pos.x += win->getCWin()->xinfo->wsr.x;
	pos.y += win->getCWin()->xinfo->wsr.y;
	return pos;
}

gpos_t Widget::getInsidePos(int32_t screenX, int32_t screenY) {
	gpos_t ret;
	gpos_t pos = getScreenPos();
	ret.x = screenX - pos.x;
	ret.y = screenY - pos.y;
	return ret;
}

grect_t Widget::getRootArea() {
	gpos_t pos = getRootPos();
	return {pos.x, pos.y, area.w, area.h};
}

grect_t Widget::getScreenArea() {
	gpos_t pos = getScreenPos();
	return {pos.x, pos.y, area.w, area.h};
}

}
