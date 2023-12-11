#include <Widget/WidgetWin.h>

namespace Ewok {

void WidgetWin::onRepaint(graph_t* g) {
	rootWidget.repaint(g);
}

void WidgetWin::onResize(void) {
	rootWidget.resizeTo(xwin->xinfo->wsr.w, xwin->xinfo->wsr.h);
}	

void WidgetWin::onEvent(xevent_t* ev) {
	rootWidget.sendEvent(ev);
	rootWidget.updateWin();
}

}

