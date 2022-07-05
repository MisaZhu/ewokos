#ifndef WIDGET_HH
#define WIDGET_HH

#include <x++/XWin.h>

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

	virtual void onRepaint(graph_t* g, grect_t* grect) { }
	virtual void onResize() { }
	virtual void onMove() { }
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
		Widget* ff = getForeFather();
		if(ff != NULL)
			ff->repaintWin();
	}

	virtual void repaintWin(void) {};
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
	
	inline grect_t* getRect() { return &rect; }
};

}

#endif