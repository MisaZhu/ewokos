#include <Widget/WidgetWin.h>
#include <ewoksys/timer.h>

namespace Ewok {

WidgetWin::WidgetWin() {
	root = NULL;
	theme = Theme::loadDefault();
	timerID = 0;
	painting = false;
}

WidgetWin::~WidgetWin() {
	if(root != NULL)
		delete root;
	if(theme != NULL)
		delete theme;
	if(timerID > 0)
		timer_remove(timerID);
}

void WidgetWin::setTheme(Theme* theme)  {
	if(this->theme != NULL)
		delete this->theme;
	this->theme = theme;
}

void WidgetWin::onRepaint(graph_t* g) {
	if(root == NULL)
		return;
	if(painting)
		return;
	painting = true;
	root->repaint(g, theme);
	painting = false;
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

void WidgetWin::timerTask() {
	if(root == NULL)
		return;
	root->onTimer();
	root->repaintWin();
}

void WidgetWin::setRoot(RootWidget* root) {
	root->xwin = this;
	this->root = root;
}

static WidgetWin* _win = NULL;

static void _timerHandler(void) {
	_win->timerTask();
}

void WidgetWin::setTimer(uint32_t fps) {
	if(fps == 0)
		return;

	_win = this;
	if(timerID > 0)
		timer_remove(timerID);
	timerID = timer_set(1000*1000/fps, _timerHandler);
}

}

