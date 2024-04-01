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
	marginH = 0;
	marginV = 0;
	id = _idCounter++;
	isContainer = false;
	disabled = false;
	themePrivate = NULL;
	visible = true;
}

Widget::~Widget(void)  { 
	if(themePrivate != NULL)
		delete themePrivate;
}

void Widget::setTheme(XTheme* theme)  {
	if(themePrivate != NULL)
		delete themePrivate;
	themePrivate = theme;
	update();
}

bool Widget::onMouse(xevent_t* ev) {
	return false;
}

bool Widget::onIM(xevent_t* ev) {
	return false;
}

bool Widget::onEvent(xevent_t* ev) { 
	if(disabled)
		return false;

	if(ev->type == XEVT_MOUSE) {
		grect_t r = getScreenArea();
		if(ev->value.mouse.x > r.x && ev->value.mouse.x < (r.x+r.w) &&
				ev->value.mouse.y > r.y && ev->value.mouse.y < (r.y+r.h)) {
			if(ev->state == XEVT_MOUSE_DOWN) {
				getRoot()->focus(this);
			}
			return onMouse(ev);
		}
	}
	else if(ev->type == XEVT_IM && getRoot()->getFocused() == this) {
		return onIM(ev);
	}
	return false; 
}

RootWidget* Widget::getRoot(void) {
	if(father == NULL)
		return NULL;

	Container* wd = father;
	while(wd != NULL && wd->father != NULL)
		wd = wd->father;
	return (RootWidget*)wd;
}

void Widget::repaint(graph_t* g, XTheme* theme) {
	if(!dirty || !visible)
		return;
	if(this->themePrivate != NULL)
		theme = this->themePrivate;

	grect_t r = getRootArea();
	graph_set_clip(g, r.x, r.y, r.w, r.h);
	onRepaint(g, theme, r);
	dirty = false;
}

void Widget::update() {
	dirty = true;
	if(father != NULL) {
		if(isAlpha())
			father->update();
	}

	RootWidget* root = getRoot();
	if(root != NULL)
		root->refresh();
}

void Widget::disable() {
	disabled = true;
	update();
}

void Widget::enable() {
	disabled = false;
	update();
}

bool Widget::focused() {
	RootWidget* root = getRoot();
	if(root == NULL)
		return false;	
	return this == root->getFocused();
}

gsize_t Widget::getMinSize(void) {
	gsize_t sz = { marginH*2, marginV*2 };
	return sz;
}

void Widget::fix(uint32_t w, uint32_t h) {
	setFixed(true);
	resizeTo(w, h);
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

void Widget::show() {
	visible = true;
	if(father != NULL) {
		father->layout();
		father->update();
	}
}

void Widget::hide() {
	visible = false;
	if(father != NULL) {
		father->layout();
		father->update();
	}
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
