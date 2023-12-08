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
		if(wd->fixed) {
			wd->setArea(0, h, area.w, wd->area.h);
		}
		else {
			wd->setArea(0, h, area.w, asize);
		}
		h += wd->area.h;
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
		if(wd->fixed) {
			wd->setArea(w, 0, wd->area.w, area.h);
		}
		else {
			wd->setArea(w, 0, asize, area.h);
		}
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

void  Container::repaint(graph_t* g) {
	if(dirty)
		onRepaint(g);

	Widget* wd = children;
	while(wd != NULL) {
		if(dirty)
			wd->dirty = true;
		wd->repaint(g);
		wd = wd->next;
	}
	dirty = false;
}

Container::Container() {
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