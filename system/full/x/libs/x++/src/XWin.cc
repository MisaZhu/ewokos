#include "x++/XWin.h"
#include <stdio.h>
#include <string.h>
#include <font/font.h>

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

static void _on_move(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doMove();
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

static void _on_reorg(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doReorg();
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
	font_init();
	this->xwin = NULL;
}

void XWin::close() {
	if(xwin == NULL)
		return;
	xwin_close(xwin);
	xwin = NULL;
}

bool XWin::setVisible(bool visible) {
	if(xwin == NULL)
		return false;
	xwin_set_visible(xwin, visible);
	return true;
}

void XWin::setAlpha(bool alpha) {
	if(xwin == NULL)
		return;
	xwin_set_alpha(xwin, alpha);
}

void XWin::top() {
	if(xwin == NULL)
		return;
	xwin_top(xwin);
}

void XWin::pop() {
	if(xwin == NULL)
		return;
	xwin_set_visible(xwin, true);
	xwin_top(xwin);
}

bool XWin::callXIM(void) {
	if(xwin == NULL)
		return false;
	xwin_call_xim(xwin);
	return true;
}

bool XWin::getInfo(xinfo_t& xinfo) {
	if(xwin == NULL || xwin->xinfo == NULL)	
		return false;
	memcpy(&xinfo, xwin->xinfo, sizeof(xinfo_t));
	return true;
}

void XWin::repaint(void) {
	if(xwin == NULL)	
		return;
	xwin_repaint(xwin);
}

void XWin::resize(int dw, int dh) {
	if(xwin == NULL)	
		return;
	xwin_resize(xwin, dw, dh);
}

void XWin::resizeTo(int w, int h) {
	if(xwin == NULL)	
		return;
	xwin_resize_to(xwin, w, h);
}

void XWin::move(int dx, int dy) {
	if(xwin == NULL)	
		return;
	xwin_move(xwin, dx, dy);
}

void XWin::moveTo(int x, int y) {
	if(xwin == NULL)	
		return;
	xwin_move_to(xwin, x, y);
}

void XWin::setDisplay(int index) {
	if(xwin == NULL)	
		return;
	xwin_set_display(xwin, index);
}

gpos_t XWin::getInsidePos(int32_t x, int32_t y) {
	gpos_t pos;
	pos.x = x - xwin->xinfo->wsr.x;
	pos.y = y - xwin->xinfo->wsr.y;
	return pos;
}

gpos_t XWin::getAbsPos(int32_t x, int32_t y) {
	gpos_t pos;
	pos.x = x + xwin->xinfo->wsr.x;
	pos.y = y + xwin->xinfo->wsr.y;
	return pos;
}

void XWin::setCWin(xwin_t* xw) {
	xw->data = this;
	xwin = xw;
	xwin->on_close = _on_close;
	xwin->on_repaint = _on_repaint;
	xwin->on_min = _on_min;
	xwin->on_resize = _on_resize;
	xwin->on_move = _on_move;
	xwin->on_focus = _on_focus;
	xwin->on_unfocus = _on_unfocus;
	xwin->on_reorg = _on_reorg;
	xwin->on_event = _on_event;
}
