#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <ewoksys/timer.h>

namespace Ewok {

WidgetWin::WidgetWin() {
	root = NULL;
	timerFPS = 16;
	lastTimerTick = 0;
	widgetRegWin(this);

	RootWidget* r = new RootWidget();
	r->setType(Container::VERTICAL);
	setRoot(r);
}

WidgetWin::~WidgetWin() {
	if(root != NULL)
		delete root;
	widgetUnregWin(this);
}

void WidgetWin::onRepaint(graph_t* g) {
	if(root == NULL)
		return;
	if(xwin->xinfo != NULL && xwin->xinfo->update_theme)
		root->dirty = true;

	if(root->isAlpha())
		graph_clear(g, 0);
	root->repaint(g, &theme);
}

void WidgetWin::onShow(void) {
	if(root)
		root->update();
}

void WidgetWin::onMove(void) {
	if(root->isAlpha())
		root->update();
}

void WidgetWin::onFocus(void) {
	if(root == NULL)
		return;
	root->onFocus();
	root->update();
}

void WidgetWin::onUnfocus(void) {
	if(root == NULL)
		return;
	root->onUnfocus();
	root->update();
}

void WidgetWin::onResize(void) {
	if(root == NULL)
		return;
	root->resizeTo(xwin->xinfo->wsr.w, xwin->xinfo->wsr.h);
}	

void WidgetWin::onEvent(xevent_t* ev) {
	if(root == NULL)
		return;
	if(root->sendEvent(ev))
		root->refresh();
}

bool WidgetWin::onClose() {
	return true;
}

void WidgetWin::doTimer(uint64_t tick) {
	if(root == NULL || timerFPS == 0)
		return;

	if((tick - lastTimerTick) < (1000 / timerFPS))
		return;
	lastTimerTick = tick;
	root->doTimer(timerFPS);
}

void WidgetWin::setRoot(RootWidget* root) {
	if(this->root != NULL)
		delete this->root;
	root->xwin = this;
	this->root = root;
	setAlpha(root->isAlpha());
}

void WidgetWin::build() {
	onBuild();
}

void WidgetWin::onBuild() {
	RootWidget* r = new RootWidget();
	r->setType(Container::VERTICAL);
	r->setAlpha(false);
	setRoot(r);
}

void WidgetWin::setTimer(uint32_t fps) {
	if(fps < TIMER_MIN_FPS)
		fps = TIMER_MIN_FPS;
	timerFPS = fps;
	widgetXSetTimerFPS(timerFPS);
}

}

