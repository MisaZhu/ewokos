#include "x++/XWin.h"
#include <stdio.h>

using namespace Ewok;

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

XWin::XWin(void) {
	this->xwin = NULL;
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

bool XWin::callXIM(void) {
	if(xwin == NULL)
		return false;
	x_call_xim(xwin);
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

void XWin::repaint(bool thread) {
	if(xwin == NULL)	
		return;
	x_repaint(xwin, thread);
}

void XWin::resize(int dw, int dh) {
	if(xwin == NULL)	
		return;
	x_resize(xwin, dw, dh);
}

void XWin::resizeTo(int w, int h) {
	if(xwin == NULL)	
		return;
	x_resize_to(xwin, w, h);
}

void XWin::move(int dx, int dy) {
	if(xwin == NULL)	
		return;
	x_move(xwin, dx, dy);
}

void XWin::moveTo(int x, int y) {
	if(xwin == NULL)	
		return;
	x_move_to(xwin, x, y);
}

void XWin::setCWin(xwin_t* xw) {
	xwin = xw;
	xwin->on_close = _on_close;
	xwin->on_repaint = _on_repaint;
	xwin->on_min = _on_min;
	xwin->on_resize = _on_resize;
	xwin->on_focus = _on_focus;
	xwin->on_unfocus = _on_unfocus;
	xwin->on_event = _on_event;
}
