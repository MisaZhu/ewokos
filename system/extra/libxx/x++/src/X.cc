#include "x++/X.h"

static void _on_repaint(x_t* xp, graph_t* g) {
	if(xp == NULL || g == NULL)
		return;
	X* x = (X*)xp->data;
	if(x == NULL)
		return;
	x->__doRepaint(g);
}

static void _on_min(x_t* xp) {
	if(xp == NULL)
		return;
	X* x = (X*)xp->data;
	if(x == NULL)
		return;
	x->__doMin();
}

static void _on_resize(x_t* xp) {
	if(xp == NULL)
		return;
	X* x = (X*)xp->data;
	if(x == NULL)
		return;
	x->__doResize();
}

static void _on_focus(x_t* xp) {
	if(xp == NULL)
		return;
	X* x = (X*)xp->data;
	if(x == NULL)
		return;
	x->__doFocus();
}

static void _on_unfocus(x_t* xp) {
	if(xp == NULL)
		return;
	X* x = (X*)xp->data;
	if(x == NULL)
		return;
	x->__doUnfocus();
}

static void _on_loop(x_t* xp) {
	if(xp == NULL)
		return;
	X* x = (X*)xp->data;
	if(x == NULL)
		return;
	x->__doLoop();
}

static void _on_event(x_t* xp, xevent_t* ev) {
	if(xp == NULL)
		return;
	X* x = (X*)xp->data;
	if(x == NULL)
		return;
	x->__doEvent(ev);
}

bool X::open(int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style) {
	if(xp != NULL)
		x_close(xp);

	xp = x_open(x, y, w, h, title, style);
	if(xp == NULL)
		return false;
	xp->data = this;
	xp->on_repaint = _on_repaint;
	xp->on_min = _on_min;
	xp->on_resize = _on_resize;
	xp->on_focus = _on_focus;
	xp->on_unfocus = _on_unfocus;
	xp->on_loop = _on_loop;
	xp->on_event = _on_event;
	return true;
}

void X::close() {
	if(xp != NULL) {
		xp->closed = true;
	}
}

bool X::setVisible(bool visible) {
	if(xp == NULL)
		return false;
	x_set_visible(xp, visible);
	return true;
}

void X::run() {
	if(xp == NULL)
		return;
	
	x_run(xp);

	x_close(xp);
	xp = NULL;
}

bool X::updateInfo(const xinfo_t& xinfo) {
	if(xp == NULL)	
		return false;
	return (x_update_info(xp, &xinfo) == 0);
}

bool X::getInfo(xinfo_t& xinfo) {
	if(xp == NULL)	
		return false;
	return (x_get_info(xp, &xinfo) == 0);
}

bool X::screenInfo(xscreen_t& scr) {
	return (x_screen_info(&scr) == 0);
}

bool X::isTop(void) {
	if(xp == NULL)	
		return false;
	return (x_is_top(xp) == 0);
}

void X::repaint() {
	if(xp == NULL)	
		return;
	x_repaint(xp);
}
