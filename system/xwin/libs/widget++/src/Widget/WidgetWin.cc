#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <ewoksys/timer.h>

namespace Ewok {

WidgetWin::WidgetWin() {
	root = NULL;
	timerFPS = 16;
	timerSteps = 0;
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
	if(root == NULL || g == NULL)
		return;
	if(xwin->xinfo != NULL && xwin->xinfo->update_theme)
		root->markFull();
	if(theme.getFont() == NULL)
		theme.loadSystem();

	if(root->isAlpha()) {
		if(root->isPartialRepaint() && !root->isDmgFull()) {
			/* damage-rect mode: no wipe. Widgets raw-write their own
			 * background over the damage; untouched pixels (e.g. the
			 * terminal canvas a scroll blit reads) must survive. */
		}
		else {
			/* alpha windows clear the whole workspace before repainting.
			   A queued second repaint after the first full draw would otherwise
			   wipe the buffer and skip child painting because their dirty flags
			   were already consumed by the previous pass. */
			root->dirty = true;
			graph_clear(g, 0);
		}
	}
	root->repaint(g, &theme);
	/*hand the damage just rendered to libx so the compositor can shrink its
	 * recomposite to the band; anything but a clean partial rect claims the
	 * whole frame (full redraws, non-partial apps) */
	if(root->isPartialRepaint() && root->hasDamage() && !root->isDmgFull())
		xwin_set_damage(xwin, &root->getDamage());
	else
		xwin_set_damage(xwin, NULL);
	if(root->isPartialRepaint())
		root->clearDamage();
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
	onTimer(timerFPS, timerSteps);
	root->doTimer(timerFPS, timerSteps);
	timerSteps++;
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
