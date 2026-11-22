#ifndef ROOT_WIDGET_HH
#define ROOT_WIDGET_HH

#include <x++/XWin.h>
#include <Widget/Container.h>

namespace Ewok {

class WidgetWin;
class RootWidget: public Container {
	WidgetWin* xwin;
	bool doRefresh;
	Widget* focusedWidget;
	Widget* dragedWidget;
	Widget* touchedWidget;              /* owner of the multi-touch gesture in progress */
	touch_evt_t touchs[X_TOUCH_MAX];    /* last known state of each contact, by id */
	uint32_t touchMask;                 /* contacts currently down, one bit per id */

	/* Opt-in damage-rect repaint: updates accumulate a damage rect and only
	 * the widgets intersecting it repaint (no full-canvas wipe), instead of
	 * the legacy full redraw every frame. Structural changes mark it full. */
	bool partialRepaint;
	bool dmgValid;
	bool dmgFull;
	grect_t dmg;

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
public:
	friend WidgetWin;
	RootWidget();

	RootWidget* getRoot(void);
	inline WidgetWin* getWin() { return xwin; }
	inline void setWin(WidgetWin* xwin) { this->xwin = xwin; }
	inline void refresh() { doRefresh = true; }

	void setPartialRepaint(bool enable);
	inline bool isPartialRepaint() { return partialRepaint; }
	void addDamage(const grect_t& r);
	void markFull();
	inline bool hasDamage() { return dmgValid && dmg.w > 0 && dmg.h > 0; }
	inline bool isDmgFull() { return dmgFull; }
	inline const grect_t& getDamage() { return dmg; }
	void clearDamage();

	void focus(Widget* wd);
	void onFocus();
	void release(Widget* wd);
	inline Widget* getFocused() { return focusedWidget; }
	inline Widget* getDraged() { return dragedWidget; }
	inline void drag(Widget* wd) { dragedWidget = wd; }
	inline Widget* getTouched() { return touchedWidget; }
	inline void touch(Widget* wd) { touchedWidget = wd; }

	/* contacts currently down on this window, tracked across the
	   XEVT_TOUCH_MT frames (a frame only carries what changed) */
	uint32_t getTouchNum();
	const touch_evt_t* getTouch(uint32_t id);   /* NULL while that contact is up */

	void repaintWin();
	void update();
	bool sendEvent(xevent_t* ev);
};

}

#endif
