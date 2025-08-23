#ifndef WIDGET_HH
#define WIDGET_HH

#include <x++/XWin.h>
#include <string.h>
#include <tinyjson/tinyjson.h>
#include <ewoksys/klog.h>
#include <UniObject/UniObject.h>

namespace Ewok {

typedef struct {
	bool tick;
	uint32_t fps;
	uint32_t step;
} TimerT;

class Container;
class RootWidget;
class Stage;
class WidgetWin;
class Widget;

typedef void (*WidgetEventFuncT)(Widget* wd, xevent_t* evt, void* arg);
class Widget :public UniObject {
	Widget* next;
	Widget* prev;

	bool beContainer;
	bool beRoot;
protected:
	XTheme* themePrivate;
	uint32_t id;
	string   name;
	int32_t marginH;
	int32_t marginV;

	TimerT timerTick;
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
	virtual void doTimer();
	virtual void timerTrigger(uint32_t timerFPS, uint32_t timerStep);
	virtual void onTimer(uint32_t timerFPS, uint32_t timerStep) { }
	virtual void onFocus() { }
	virtual void onUnfocus() { }
	virtual void onAdd() { }
	virtual bool onEvent(xevent_t* ev);
	virtual void setAttr(const string& attr, json_var_t*value);
	virtual json_var_t* getAttr(const string& attr);

	WidgetEventFuncT onEventFunc;
	void* onEventFuncArg;
public:
	friend Container;
	friend RootWidget;
	friend Stage;

	inline void setEventFunc(WidgetEventFuncT func, void* arg = NULL) {
		onEventFunc = func;
		onEventFuncArg = arg;
	}

	Widget(void);
	virtual ~Widget(void);

	void setAlpha(bool alpha);

	inline bool isContainer() { return beContainer; }
	inline bool isRoot() { return beRoot; }

	inline void setMarginH(int32_t v) { marginH = v; }
	inline void setMarginV(int32_t v) { marginV = v; }
	inline void setFixed(bool fixed) { this->fixed = fixed; }
	inline bool isAlpha() { return alpha; }
	inline uint32_t getID() { return id; }
	inline void setID(uint32_t id) { this->id = id; }
	inline const string& getName() { return name; }
	inline void setName(const string& name) { this->name = name; }
	inline XTheme* getTheme() { return themePrivate; }

	void dupTheme(XTheme* theme);
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

	bool isFixed() { return fixed; }
	bool isVisible() { return visible; }
	Widget* getNext() { return next; }
	Widget* getPrev() { return prev; }
	Container* getRootContainer(void);
	virtual RootWidget* getRoot(void);
	WidgetWin*  getWin(void);
	gpos_t getRootPos(int32_t x = 0, int32_t y = 0);
	gpos_t getScreenPos(int32_t x = 0, int32_t y = 0);
	gpos_t getInsidePos(int32_t screenX, int32_t screenY);
	grect_t getRootArea(bool margin = true);
	grect_t getScreenArea(bool margin = true);
	bool   focused();
	Container* getFather() { return father; }

	virtual gsize_t getMinSize(void);
	void update();
};

}

#endif