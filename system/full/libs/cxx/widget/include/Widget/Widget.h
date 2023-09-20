#ifndef WIDGET_HH
#define WIDGET_HH

#include <x++/XWin.h>
#include <string.h>
#include <sys/klog.h>

namespace Ewok {

class Container;
class Widget {
	friend Container;
	Widget* next;
	Widget* prev;
protected:
	Widget* father;
	bool dirty;

	bool fixed;
	grect_t rect;

	virtual void onRepaint(graph_t* g, grect_t* grect) {
		(void)g;
		(void)grect;
	}

	virtual void onResize() { }
	virtual void onMove() { }

	virtual bool onMouse(xevent_t* ev) {  return false; }
	virtual bool onKey(xevent_t* ev) {  return false; }
	virtual bool onEvent(xevent_t* ev) { 
		if(ev->type == XEVT_MOUSE) {
			grect_t r;
			getAbsRect(&r);
			if(ev->value.mouse.x > r.x && ev->value.mouse.x < (r.x+r.w) &&
					ev->value.mouse.y > r.y && ev->value.mouse.y < (r.y+r.h))
				return onMouse(ev);
		}
		return false; 
	}

	virtual XWin* getWin() {
		if(father == NULL)
			return NULL;
		return father->getWin();
	}

public:
	inline Widget(void)  { 
		dirty = true;
		fixed = false;
		father = NULL;
		next = NULL;
		prev = NULL;
		rect = {0, 0, 0, 0};
	}

	virtual ~Widget() { }

	inline Widget* getForeFather(void) {
		Widget* wd = father;
		while(wd != NULL && wd->father != NULL)
			wd = wd->father;
		return wd;
	}

	inline void update() {
		dirty = true;
		XWin* win = getWin();
		if(win != NULL)
			win->repaint();
	}

	virtual void repaint(graph_t* g, grect_t* grect) {
		if(!dirty)
			return;
		onRepaint(g, grect);
		dirty = false;
	}

	inline void resizeTo(int w, int h) {
		if(rect.w == w && rect.h == h)
			return;
		rect.w = w;
		rect.h = h;
		onResize();
		update();
		if(father)
			father->update();
	}

	inline void resize(int dw, int dh) {
		resizeTo(rect.w + dw, rect.h + dh);
	}

	inline void moveTo(int x, int y) {
		if(rect.x == x && rect.y == y)
			return;
		rect.x = x;
		rect.y = y;
		onMove();
		update();
		if(father)
			father->update();
	}

	inline void move(int dx, int dy) {
		moveTo(rect.x + dx, rect.y + dy);
	}

	inline void setFixed(bool fixed) {
		this->fixed = fixed;
	}

	inline void setRect(int x, int y, int w, int h) {
		resizeTo(w, h);
		moveTo(x, y);
	}
	
	inline void getRect(grect_t* r) { memcpy(r, &rect, sizeof(grect_t)); }

	inline void getAbsRect(grect_t* r) {
		getRect(r);
		Widget* wd = father;
		while(wd != NULL) {
			r->x += wd->rect.x;
			r->y += wd->rect.y;
			wd = wd->father;
		}

		XWin* win = getWin();
		if(win == NULL)
			return;
		r->x += win->getCWin()->xinfo->wsr.x;
		r->y += win->getCWin()->xinfo->wsr.y;
	}
};

}

#endif