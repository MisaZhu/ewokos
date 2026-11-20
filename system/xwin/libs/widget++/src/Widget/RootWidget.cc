#include <Widget/RootWidget.h>
#include <Widget/WidgetWin.h>

namespace Ewok {

RootWidget::RootWidget() {
	doRefresh = false;
	focusedWidget = NULL;
	dragedWidget= NULL;
	touchedWidget = NULL;
	touchMask = 0;
	memset(touchs, 0, sizeof(touchs));
	xwin = NULL;
	alpha = false;
	beRoot = true;
	partialRepaint = false;
	dmgValid = false;
	dmgFull = true;
	dmg = {0, 0, 0, 0};
}

void RootWidget::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	if(color_a(theme->basic.bgColor) != 0xff)
		setAlpha(true);

	//if(!theme->basic.noWindowBG || !alpha)
	graph_fill_rect(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
}
 
RootWidget* RootWidget::getRoot(void) {
	return this;
}

void RootWidget::focus(Widget* wd) {
	if(wd == NULL) {
		focusedWidget = NULL;
		return;
	}

	Widget* oldFocusedWidget = focusedWidget;
	focusedWidget = wd;

	if(oldFocusedWidget != NULL && oldFocusedWidget != wd) {
		oldFocusedWidget->onUnfocus();
		oldFocusedWidget->update();
	}

	wd->onFocus();
	wd->update();
}

void RootWidget::onFocus() {
	if(this->focusedWidget == NULL)
		return;
	this->focusedWidget->onFocus();
}

void RootWidget::release(Widget* wd) {
	if(wd == NULL)
		return;

	if(focusedWidget == wd ||
			(wd->isContainer() && ((Container*)wd)->has(focusedWidget))) {
		focusedWidget = NULL;
	}

	if(dragedWidget == wd ||
			(wd->isContainer() && ((Container*)wd)->has(dragedWidget))) {
		dragedWidget = NULL;
	}

	if(touchedWidget == wd ||
			(wd->isContainer() && ((Container*)wd)->has(touchedWidget))) {
		touchedWidget = NULL;
	}
}

void RootWidget::repaintWin() { 
	if(xwin == NULL)
		return;
	if(doRefresh) {
		xwin->repaint();
		doRefresh = false;
	}
}

void RootWidget::update() {
	markFull();
}

void RootWidget::setPartialRepaint(bool enable) {
	partialRepaint = enable;
	markFull();
}

void RootWidget::addDamage(const grect_t& r) {
	if(r.w <= 0 || r.h <= 0)
		return;
	/* stay clean: a dirty root would fill its theme background over the
	 * whole damage and force every child to repaint, defeating the point */
	doRefresh = true;
	if(dmgFull)
		return;
	if(!dmgValid) {
		dmg = r;
		dmgValid = true;
		return;
	}
	/* union */
	int32_t x1 = dmg.x < r.x ? dmg.x : r.x;
	int32_t y1 = dmg.y < r.y ? dmg.y : r.y;
	int32_t x2 = (dmg.x + dmg.w) > (r.x + r.w) ? (dmg.x + dmg.w) : (r.x + r.w);
	int32_t y2 = (dmg.y + dmg.h) > (r.y + r.h) ? (dmg.y + dmg.h) : (r.y + r.h);
	dmg.x = x1;
	dmg.y = y1;
	dmg.w = x2 - x1;
	dmg.h = y2 - y1;
}

void RootWidget::markFull() {
	dirty = true;
	doRefresh = true;
	dmgFull = true;
	dmgValid = true;
	dmg = getRootArea(false);
}

void RootWidget::clearDamage() {
	dmgValid = false;
	dmgFull = false;
	dmg = {0, 0, 0, 0};
}

uint32_t RootWidget::getTouchNum() {
	uint32_t num = 0;
	for(uint32_t i = 0; i < X_TOUCH_MAX; i++) {
		if((touchMask & (1u << i)) != 0)
			num++;
	}
	return num;
}

const touch_evt_t* RootWidget::getTouch(uint32_t id) {
	if(id >= X_TOUCH_MAX || (touchMask & (1u << id)) == 0)
		return NULL;
	return &touchs[id];
}

bool RootWidget::sendEvent(xevent_t* ev) {
	if(ev->type == XEVT_MOUSE && ev->state == MOUSE_STATE_DOWN) {
		/* the server only presses a mouse button with at most one finger
		   down and no multi-touch gesture running, so any contact still
		   tracked here missed its release (the gesture moved on to another
		   window). The frame of the pressing finger follows this event. */
		touchMask = 0;
		touchedWidget = NULL;
	}
	if(ev->type != XEVT_TOUCH_MT)
		return onEvent(ev);

	int32_t num = ev->state;
	if(num > X_TOUCH_MAX)
		num = X_TOUCH_MAX;
	for(int32_t i = 0; i < num; i++) {
		const touch_evt_t* t = &ev->value.touch.touchs[i];
		if(t->id >= X_TOUCH_MAX)
			continue;
		if(t->state == TOUCH_STATE_RELEASE) {
			touchMask &= ~(1u << t->id);
		}
		else {
			touchs[t->id] = *t;
			touchMask |= (1u << t->id);
		}
	}

	/* the gesture stays with the widget that took its first frame, wherever
	   the fingers travel, until the last one lifts */
	bool ret = false;
	if(touchedWidget != NULL)
		ret = touchedWidget->onTouch(ev);
	else
		ret = onEvent(ev);
	if(touchMask == 0)
		touchedWidget = NULL;
	return ret;
}

}
