#ifndef WIDGET_CONTAINER_HH
#define WIDGET_CONTAINER_HH

#include <Widget/Widget.h>
#include <sys/klog.h>

namespace Ewok {

class Container: public Widget {
	Widget* children;
	Widget* childrenEnd;
	uint32_t num;
	int type;

	void layoutV() {
		int asize = rect.h;
		int anum = 0;
		Widget* wd = children;
		while(wd != NULL) {
			if(wd->fixed)
				asize -= wd->rect.h;
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
				wd->setRect(0, h, rect.w, wd->rect.h);
			}
			else {
				wd->setRect(0, h, rect.w, asize);
			}
			h += wd->rect.h;
			wd = wd->next;
		}
	}

	void layoutH() {
		int asize = rect.w;
		int anum = 0;
		Widget* wd = children;
		while(wd != NULL) {
			if(wd->fixed)
				asize -= wd->rect.w;
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
				wd->setRect(w, 0, wd->rect.w, rect.h);
			}
			else {
				wd->setRect(w, 0, asize, rect.h);
			}
			w += wd->rect.w;
			wd = wd->next;
		}
	}

	void layout() {
		if(type == FIXED)
			return;
		if(type == VERTICLE)
			layoutV();
		else if(type == HORIZONTAL)
			layoutH();
	}

protected:
	void onResize() { 
		layout();
	}

	bool onEvent(xevent_t* ev) final {
		Widget* wd = childrenEnd;
		while(wd != NULL) {
			if(wd->onEvent(ev)) {
				return true;
			}
			wd = wd->prev;
		}
		return false;
	}

public:
	static const int FIXED = 0;
	static const int VERTICLE = 1;
	static const int HORIZONTAL = 2;

	void add(Widget* child) {
		child->father = this;
		if(childrenEnd != NULL) {
			childrenEnd->next = child;
			child->prev = childrenEnd;
		}
		else
			children = child;
		childrenEnd = child;
		num++;
	}

	virtual void repaint(graph_t* g, grect_t* grect) {
		if(dirty) {
			onRepaint(g, grect);
		}
		Widget* wd = children;
		while(wd != NULL) {
			if(dirty)
				wd->dirty = true;
			grect_t r = {
				wd->rect.x+grect->x,
				wd->rect.y+grect->y,
				wd->rect.w,
				wd->rect.h};
			wd->repaint(g, &r);
			wd = wd->next;
		}
	}

	inline Container() {
		children = NULL;
		childrenEnd = NULL;
		type = FIXED;
		num = 0;
	}

	inline void setType(int type) {
		this->type = type;
	}

	void clear() {
		Widget* wd = children;
		while(wd != NULL) {
			Widget* next = wd->next;
			delete wd;
			wd = next;
		}
		children = NULL;
		num = 0;
	}

	inline virtual ~Container() {
		clear();
	}
};

}

#endif