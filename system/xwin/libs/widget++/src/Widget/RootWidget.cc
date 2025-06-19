#include <Widget/RootWidget.h>
#include <Widget/WidgetWin.h>

namespace Ewok {

RootWidget::RootWidget() {
	doRefresh = false;
	focusedWidget = NULL;
	dragedWidget= NULL;
	xwin = NULL;
	alpha = true;
}

void RootWidget::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	if(!alpha)
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
}
 
RootWidget* RootWidget::getRoot(void) {
	return this;
}

void RootWidget::focus(Widget* wd) {
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

void RootWidget::repaintWin() { 
	if(xwin == NULL)
		return;
	if(doRefresh) {
		xwin->repaint();
		doRefresh = false;
	}
}

void RootWidget::update() {
	dirty = true;
	doRefresh = true;
}

bool RootWidget::sendEvent(xevent_t* ev) {
	return onEvent(ev);
}

}