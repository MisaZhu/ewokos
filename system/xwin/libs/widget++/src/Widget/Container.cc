#include <Widget/Container.h>
#include <Widget/RootWidget.h>

namespace Ewok {

void Container::layoutV() {
	int asize = area.h;
	int anum = 0;
	Widget* wd = children;
	while(wd != NULL) {
		if(wd->visible) {
			if(wd->fixed)
				asize -= wd->area.h;
			else
				anum++;
		}
		wd = wd->next;
	}
	if(asize < 0)
		asize = 0;
	else if(anum > 0)
		asize = (float)asize / (float)anum;

	wd = children;
	int h = 0;
	while(wd != NULL) {
		if(wd->visible) {
			int wh = asize;
			if(wd->fixed)
				wh = wd->area.h;
			wd->setArea(0, h, area.w, wh);
			h += wh;//wd->area.h;
		}
		wd = wd->next;
	}
}

void  Container::layoutH() {
	int asize = area.w;
	int anum = 0;
	Widget* wd = children;
	while(wd != NULL) {
		if(wd->visible) {
			if(wd->fixed)
				asize -= wd->area.w;
			else
				anum++;
		}
		wd = wd->next;
	}
	if(asize < 0)
		asize = 0;
	else if(anum > 0)
		asize = (float)asize / (float)anum;

	wd = children;
	int w = 0;
	while(wd != NULL) {
		if(wd->visible) {
			int ww = asize;
			if(wd->fixed)
				ww = wd->area.w;
			wd->setArea(w, 0, ww, area.h);
			w += wd->area.w;
		}
		wd = wd->next;
	}
}

void  Container::layoutO() {
	Widget* wd = children;
	while(wd != NULL) {
		wd->setArea(0, 0, area.w, area.h);
		wd = wd->next;
	}
}

bool  Container::has(Widget* w) {
	Widget* wd = children;
	while(wd != NULL) {
		if(wd == w)
			return true;
		wd = wd->next;
	}
	return false;
}

void  Container::onLayout() {
}

void  Container::layout() {
	if(area.w <= 0 || area.h <= 0) {
		return;
	}

	if(type == FIXED) {
		onLayout();
		return;
	}
	if(type == VERTICLE)
		layoutV();
	else if(type == HORIZONTAL)
		layoutH();
	else if(type == OVERLAP)
		layoutO();
	onLayout();
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
	child->onAdd();

	layout();
}

void  Container::doTimer() {
	Widget::doTimer();

	Widget* wd = children;
	while(wd != NULL) {
		wd->doTimer();
		wd = wd->next;
	}
}

void  Container::timerTrigger(uint32_t timerFPS, uint32_t timerStep) {
	Widget* wd = children;
	while(wd != NULL) {
		wd->timerTrigger(timerFPS, timerStep);
		wd = wd->next;
	}
}

void  Container::repaint(graph_t* g, XTheme* theme) {
	if(!visible)
		return;
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
	Widget* wd = children;
	while(wd != NULL) {
		if(wd->id != 0 && wd->id == id)
			return wd;

		if(wd->isContainer) {
			Widget* w = ((Container*)wd)->get(id);
			if(w != NULL)
				return w;
		}
		wd = wd->next;
	}
	return NULL;
}

Widget* Container::get(const string& name) {
	Widget* wd = children;
	while(wd != NULL) {
		if(wd->name == name)
			return wd;

		if(wd->isContainer) {
			Widget* w = ((Container*)wd)->get(name);
			if(w != NULL)
				return w;
		}
		wd = wd->next;
	}
	return NULL;
}

Container::Container() {
	isContainer = true;
	children = NULL;
	childrenEnd = NULL;
	type = FIXED;
	alpha = true;
	num = 0;
}

void  Container::setType(uint8_t type) {
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