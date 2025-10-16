#include "x++/XWin.h"
#include "x++/X.h"
#include <stdio.h>
#include <string.h>
#include <font/font.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/klog.h>

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

static void _on_update_theme(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;

	XTheme* theme = xwin->getTheme();
	if(theme == NULL)
		return;
	theme->loadSystem();
}

static void _on_move(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doMove();
}

static bool _on_close(xwin_t* xw) {
	if(xw == NULL)
		return false;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return false;
	return xwin->__doClose();
}

static void _on_focus(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doFocus();
}

static void _on_show(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doShow();
}

static void _on_hide(xwin_t* xw) {
	if(xw == NULL)
		return;
	XWin* xwin = (XWin*)xw->data;
	if(xwin == NULL)
		return;
	xwin->__doHide();
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

bool XWin::focused() {
	if(xwin == NULL || xwin->xinfo == NULL)
		return false;
	return xwin->xinfo->focused;
}

XWin::XWin(void) {
	font_init();
	theme.loadSystem();
	displayIndex = 0;
	xwin = NULL;
	onDialogedFunc = NULL;
}

XWin::~XWin(void) {
	close();
	font_quit();
}

void XWin::close() {
	if(xwin == NULL)
		return;
	xwin_close(xwin);
	xwin_destroy(xwin);
	xwin = NULL;
}

bool XWin::open(X* xp, int32_t dispIndex, int x, int y, uint32_t w, uint32_t h,
		const char* title, uint32_t style, bool visible) {
	if(xp == NULL)
		return false;
	xscreen_info_t scr;
	X::getScreenInfo(scr, dispIndex);


	uint32_t minW = scr.size.w*2/3;
	uint32_t minH = scr.size.h*2/3;
	if(w == 0)
		w = minW + random_to(scr.size.w - minW);
	if(h == 0)
		h = minH + random_to(scr.size.h - minH - 32);

	if(x < 0) {
		x = 0;
		if(scr.size.w > w) {
			x = random_to(scr.size.w - w);
		}
	}

	if(y < 0) {
		y = 32;
		if(scr.size.h > (h+32))
			y += (int32_t)random_to(scr.size.h - (h + 32));
	}	

	xwin_t* xw = xwin_open(xp->c_x(), dispIndex, x, y, w, h, title, style);
	if(xw == NULL) {
		slog("xwin open failed\n");
		return false;
	}
	this->x = xp;
	setCWin(xw);
	onOpen();
	setVisible(visible);
	return true;
}

void XWin::dialoged(XWin* from, int res, void* arg) {
	if(onDialogedFunc != NULL)
		onDialogedFunc(this, from, res, arg);
	else
		onDialoged(from, res, arg);
}

bool XWin::open(X* xp, int32_t dispIndex, const grect_t& r, const char* title, uint32_t style, bool visible) {
	return open(xp, dispIndex, r.x, r.y, r.w, r.h, title, style, visible);
}

bool XWin::setVisible(bool visible) {
	if(xwin == NULL)
		return false;

	xwin_set_visible(xwin, visible);
	if(visible) 
		xwin_repaint(xwin);
	return true;
}

void XWin::setAlpha(bool alpha) {
	if(xwin == NULL)
		return;
	xwin_set_alpha(xwin, alpha);
}

bool XWin::isAlpha(void) {
	if(xwin == NULL || xwin->xinfo == NULL)
		return false;
	return xwin->xinfo->alpha;
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

bool XWin::callXIM(bool show) {
	if(xwin == NULL)
		return false;
	if(xwin_call_xim(xwin, show) == 0)
		return true;
	return false;
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

void XWin::max(void) {
	if(xwin == NULL)	
		return;
	xwin_max(xwin);
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

void XWin::busy(bool bs) {
	if(xwin == NULL)	
		return;
	xwin_busy(xwin, bs);
}

gpos_t XWin::getInsidePos(int32_t x, int32_t y) {
	gpos_t pos;
	pos.x = x - xwin->xinfo->wsr.x;
	pos.y = y - xwin->xinfo->wsr.y;
	return pos;
}

gpos_t XWin::getScreenPos(int32_t x, int32_t y) {
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
	xwin->on_show = _on_show;
	xwin->on_hide = _on_hide;
	xwin->on_unfocus = _on_unfocus;
	xwin->on_reorg = _on_reorg;
	xwin->on_event = _on_event;
	xwin->on_update_theme = _on_update_theme;
}
