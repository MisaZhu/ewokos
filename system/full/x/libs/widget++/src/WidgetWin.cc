#include <Widget/WidgetWin.h>

namespace Ewok {

void WidgetWin::onRepaint(graph_t* g) {
	if(root == NULL)
		return;
	root->repaint(g, root->theme);
}

void WidgetWin::onResize(void) {
	if(root == NULL)
		return;
	root->resizeTo(xwin->xinfo->wsr.w, xwin->xinfo->wsr.h);
}	

void WidgetWin::onEvent(xevent_t* ev) {
	if(root == NULL)
		return;
	root->sendEvent(ev);
	root->repaintWin();
}

void WidgetWin::setRoot(RootWidget* root) {
	root->xwin = this;
	this->root = root;
}

}

