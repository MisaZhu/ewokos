#ifndef WIDGET_HH
#define WIDGET_HH

#include <x++/XWin.h>
#include <string.h>
#include <ewoksys/klog.h>

namespace Ewok {

class Container;
class RootWidget;
class Widget {
	Widget* next;
	Widget* prev;

	bool isContainer;
protected:
	XTheme* themePrivate;
	uint32_t id;
	int32_t marginH;
	int32_t marginV;
	Container* father;

	bool dirty;
	bool fixed;
	bool disabled;
	bool alpha;
	bool visible;

	grect_t area;

	virtual void onResize() { }
	virtual void onMove() { }
	virtual bool onMouse(xevent_t* ev);
	virtual bool onIM(xevent_t* ev);

	virtual void repaint(graph_t* g, XTheme* theme);
	virtual void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) = 0;
	virtual void onTimer() { }
	virtual void onFocus() { }
	virtual void onUnfocus() { }
	virtual bool onEvent(xevent_t* ev);
public:
	friend Container;
	friend RootWidget;

	Widget(void);
	virtual ~Widget(void);

	inline void setMarginH(int32_t v) { marginH = v; }
	inline void setMarginV(int32_t v) { marginV = v; }
	inline void setFixed(bool fixed) { this->fixed = fixed; }
	inline void setAlpha(bool alpha) { this->alpha = alpha; }
	inline bool isAlpha() { return alpha; }
	inline uint32_t getID() { return id; }
	inline void setID(uint32_t id) { this->id = id; }
	inline XTheme* getTheme() { return themePrivate; }

	void setTheme(XTheme* theme);
	void disable();
	void enable();
	void fix(const gsize_t& size);
	void fix(uint32_t w, uint32_t h);

	void resizeTo(int w, int h);
	void resize(int dw, int dh);
	void moveTo(int x, int y);
	void move(int dx, int dy);
	void setArea(int x, int y, int w, int h);
	void show();
	void hide();

	RootWidget* getRoot(void);
	gpos_t getRootPos(int32_t x = 0, int32_t y = 0);
	gpos_t getScreenPos(int32_t x = 0, int32_t y = 0);
	gpos_t getInsidePos(int32_t screenX, int32_t screenY);
	grect_t getRootArea(bool margin = true);
	grect_t getScreenArea(bool margin = true);
	bool   focused();

	virtual gsize_t getMinSize(void);
	void update();
};

}

#endif