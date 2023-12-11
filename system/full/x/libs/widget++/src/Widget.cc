#include <Widget/Widget.h>
#include <Widget/RootWidget.h>

namespace Ewok {

static uint32_t _idCounter = 0;
Widget::Widget(void)  { 
	dirty = true;
	fixed = false;
	alpha = false;
	father = NULL;
	next = NULL;
	prev = NULL;
	area = {0, 0, 0, 0};
	bgColor = 0x0; //transparent
	fgColor = 0xff000000;
	marginH = 0;
	marginV = 0;
	id = _idCounter++;
	isContainer = false;
}

void Widget::onRepaint(graph_t* g, const grect_t& r) {
	if(bgColor == 0x0)
		return;
	graph_fill(g, r.x, r.y, r.w, r.h, bgColor);
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

	Container* wd = father;
	while(wd != NULL && wd->father != NULL)
		wd = wd->father;
	return (RootWidget*)wd;
}

void Widget::repaint(graph_t* g) {
	grect_t r = getRootArea();
	graph_set_clip(g, r.x, r.y, r.w, r.h);
	onRepaint(g, r);
	dirty = false;
}

void Widget::update() {
	dirty = true;
	if(father != NULL) {
		if(isAlpha())
			father->update();
	}
}

gsize_t Widget::getMinSize(void) {
	gsize_t sz = { marginH*2, marginV*2 };
	return sz;
}

void Widget::fix(uint32_t w, uint32_t h) {
	resizeTo(w, h);
	setFixed(true);
}

void Widget::fix(const gsize_t& size) {
	fix(size.w, size.h);
}

void Widget::resizeTo(int w, int h) {
	if(area.w == w && area.h == h)
		return;
	area.w = w;
	area.h = h;
	onResize();
	if(father != NULL) {
		father->layout();
		father->update();
	}
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
	if(father != NULL) {
		father->layout();
		father->update();
	}
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

grect_t Widget::getRootArea(bool margin) {
	gpos_t pos = getRootPos();
	if(margin)
		return {pos.x+marginH, pos.y+marginV, area.w-marginH*2, area.h-marginV*2};
	return {pos.x, pos.y, area.w, area.h};
}

grect_t Widget::getScreenArea(bool margin) {
	gpos_t pos = getScreenPos();
	if(margin)
		return {pos.x+marginH, pos.y+marginV, area.w-marginH*2, area.h-marginV*2};
	return {pos.x, pos.y, area.w, area.h};
}

}
