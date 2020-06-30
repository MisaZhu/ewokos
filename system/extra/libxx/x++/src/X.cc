#include "x++/X.h"

static void _on_repaint(xwin_t* xw, graph_t* g) {
	if(xw == NULL || g == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doRepaint(g);
}

static void _on_min(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doMin();
}

static void _on_resize(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doResize();
}

static void _on_close(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doClose();
}

static void _on_focus(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doFocus();
}

static void _on_unfocus(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doUnfocus();
}

static void _on_event(xwin_t* xw, xevent_t* ev) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doEvent(ev);
}

void XWin::close() {
	if(xwin == NULL)
		return;
	x_close(xwin);
	xwin = NULL;
}

bool XWin::setVisible(bool visible) {
	if(xwin == NULL)
		return false;
	x_set_visible(xwin, visible);
	return true;
}

bool XWin::updateInfo(const xinfo_t& xinfo) {
	if(xwin == NULL)	
		return false;
	return (x_update_info(xwin, &xinfo) == 0);
}

bool XWin::getInfo(xinfo_t& xinfo) {
	if(xwin == NULL)	
		return false;
	return (x_get_info(xwin, &xinfo) == 0);
}

bool XWin::screenInfo(xscreen_t& scr) {
	return (x_screen_info(&scr) == 0);
}

void XWin::repaint() {
	if(xwin == NULL)	
		return;
	x_repaint(xwin);
}

bool X::open(XWin* xwin, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style) {
	xwin_t* xw = x_open(&this->x, x, y, w, h, title, style);
	if(xw == NULL)
		return false;
	xw->data = xwin;
	xw->on_close = _on_close;
	xw->on_repaint = _on_repaint;
	xw->on_min = _on_min;
	xw->on_resize = _on_resize;
	xw->on_focus = _on_focus;
	xw->on_unfocus = _on_unfocus;
	xw->on_event = _on_event;
	xwin->setCWin(xw);
	return true;
}

X::X(void) {
	x_init(&x, this);
}

void X::run(void (*loop)(void*), void* p) {
	x.on_loop = loop;
	x_run(&x, p);
}

void X::terminate(void) {
	x.terminated = true;
}

