#include <Widget/Container.h>

namespace Ewok {

void Container::layoutV() {
	int asize = area.h;
	int anum = 0;
	Widget* wd = children;
	while(wd != NULL) {
		if(wd->fixed)
			asize -= wd->area.h;
		else
			anum++;
		wd = wd->next;
	}
	if(asize < 0)
		asize = 0;
	else if(anum > 0)
		asize = asize / anum;

	wd = children;
	int h = 0;
	while(wd != NULL) {
		int wh = asize;
		if(wd->fixed)
			wh = wd->area.h;
		if(wd->next == NULL) {
			wh = area.h - h;
			if(wd->fixed && wh < wd->area.h)
				wh = wd->area.h;
		}

		wd->setArea(0, h, area.w, wh);
		h += wh;//wd->area.h;
		wd = wd->next;
	}
}

void  Container::layoutH() {
	int asize = area.w;
	int anum = 0;
	Widget* wd = children;
	while(wd != NULL) {
		if(wd->fixed)
			asize -= wd->area.w;
		else
			anum++;
		wd = wd->next;
	}
	if(asize < 0)
		asize = 0;
	else if(anum > 0)
		asize = asize / anum;

	wd = children;
	int w = 0;
	while(wd != NULL) {
		int ww = asize;
		if(wd->fixed)
			ww = wd->area.w;
		if(wd->next == NULL) {
			ww = area.w - w;
			if(wd->fixed && ww < wd->area.w)
				ww = wd->area.w;
		}

		wd->setArea(w, 0, ww, area.h);
		w += wd->area.w;
		wd = wd->next;
	}
}

void  Container::layout() {
	if(type == FIXED)
		return;
	if(type == VERTICLE)
		layoutV();
	else if(type == HORIZONTAL)
		layoutH();
}

void  Container::onResize() { 
	layout();
}

bool  Container::onEvent(xevent_t* ev) {
	if(disabled)
		return false;
	Widget* wd = childrenEnd;
	while(wd != NULL) {
		if(wd->onEvent(ev)) {
			return true;
		}
		wd = wd->prev;
	}
	return false;
}

void  Container::add(Widget* child) {
	child->father = this;
	if(childrenEnd != NULL) {
		childrenEnd->next = child;
		child->prev = childrenEnd;
	}
	else
		children = child;
	childrenEnd = child;
	num++;
	layout();
}

void  Container::onTimer() {
	Widget* wd = children;
	while(wd != NULL) {
		wd->onTimer();
		wd = wd->next;
	}
}

void  Container::repaint(graph_t* g, const Theme* theme) {
	if(this->themePrivate != NULL)
		theme = this->themePrivate;

	if(dirty) {
		grect_t r = getRootArea();
		graph_set_clip(g, r.x, r.y, r.w, r.h);
		onRepaint(g, theme, r);
	}

	Widget* wd = children;
	while(wd != NULL) {
		if(dirty)
			wd->dirty = true;
		wd->repaint(g, theme);
		wd = wd->next;
	}
	dirty = false;
}

Widget* Container::get(uint32_t id) {
	if(this->id == id)
		return this;

	Widget* wd = children;
	while(wd != NULL) {
		if(wd->isContainer) {
			Widget* w = ((Container*)wd)->get(id);
			if(w != NULL)
				return w;
		}
		else if(wd->id == id)
			return wd;
		wd = wd->next;
	}
	return NULL;
}

Container::Container() {
	isContainer = true;
	children = NULL;
	childrenEnd = NULL;
	type = FIXED;
	num = 0;
}

void  Container::setType(int type) {
	this->type = type;
	layout();
}

void  Container::clear() {
	Widget* wd = children;
	while(wd != NULL) {
		Widget* next = wd->next;
		delete wd;
		wd = next;
	}
	children = NULL;
	num = 0;
}

Container::~Container() {
	clear();
}
}