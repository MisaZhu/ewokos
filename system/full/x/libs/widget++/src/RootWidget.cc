#include <Widget/RootWidget.h>

namespace Ewok {

RootWidget::RootWidget() {
	doRefresh = false;
	theme = Theme::load("");
}

void RootWidget::loadTheme(const char* theme) {
	if(this->theme != NULL)
		delete this->theme;
	this->theme = Theme::load(theme);
	update();
}

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