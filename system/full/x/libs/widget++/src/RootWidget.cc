#include <Widget/RootWidget.h>

namespace Ewok {

RootWidget::RootWidget() {
	doRefresh = false;
	focusedWidget = NULL;
	xwin = NULL;
}

/*void RootWidget::onRepaint(graph_t* g, const Theme* theme, const grect_t& r) {
	graph_fill(g, r.x, r.y, r.w, r.h, theme->bgColor);
}
*/

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

void RootWidget::sendEvent(xevent_t* ev) {
	if(ev->type == XEVT_MOUSE && 
			ev->state == XEVT_MOUSE_UP &&
			focusedWidget != NULL) {
		focusedWidget->sendEvent(ev); 
		setFocus(NULL);
		return;
	}
	onEvent(ev);
}

}