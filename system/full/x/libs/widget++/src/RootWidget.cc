#include <Widget/RootWidget.h>

namespace Ewok {

void RootWidget::repaintWin() { 
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
	onEvent(ev);
}

}