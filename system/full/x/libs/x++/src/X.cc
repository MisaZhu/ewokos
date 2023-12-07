#include "x++/X.h"
#include "x++/XWin.h"
#include <stdio.h>
#include <sys/basic_math.h>
#include <font/font.h>

using namespace Ewok;

static font_t* _sysFont = NULL;

bool X::open(XWin* xwin, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style) {
	xwin_t* xw = xwin_open(&this->x, x, y, w, h, title, style);
	if(xw == NULL)
		return false;
	xwin->setX(this);
	xwin->setCWin(xw);
	return true;
}

bool X::open(grect_t* desk, XWin* xwin, uint32_t w, uint32_t h, const char* title, uint32_t style) {
	uint32_t minW = desk->w/3;
	uint32_t minH = desk->h/3;
	if(w == 0)
		w = minW + random_to(desk->w - minW);
	if(h == 0)
		h = minH + random_to(desk->h - minH - 20);

	int32_t x = desk->x;
	if(desk->w > w)
		x = random_to(desk->w - w) + desk->x;

	int32_t y = 20 + desk->y;
	if(desk->h > h)
		y = 20 + random_to(desk->h - h) + desk->y;
	return open(xwin, x, y, w, h, title, style);
}

bool X::open(xscreen_t* scr, XWin* xwin, uint32_t w, uint32_t h, const char* title, uint32_t style) {
	uint32_t minW = scr->size.w/3;
	uint32_t minH = scr->size.h/3;
	if(w == 0)
		w = minW + random_to(scr->size.w - minW);
	if(h == 0)
		h = minH + random_to(scr->size.h - minH - 20);
	int32_t x = 0;
	if(scr->size.w > w)
		x = random_to(scr->size.w - w);

	int32_t y = 20;
	if(scr->size.h > h)
		y = 20 + (int32_t)random_to(scr->size.h - h);
	return open(xwin, x, y, w, h, title, style);
}

X::X(void) {
	x_init(&x, this);
}

font_t* X::getSysFont(void) {
	if(_sysFont == NULL)
		_sysFont = font_new(DEFAULT_SYSTEM_FONT, DEFAULT_SYSTEM_FONT_SIZE, true);
	return _sysFont;
}

void X::run(void (*loop)(void*), void* p) {
	x.on_loop = loop;
	x_run(&x, p);
}

void X::terminate(void) {
	x.terminated = true;
}

bool X::getScreenInfo(xscreen_t& scr, int index) {
	return (x_screen_info(&scr, index) == 0);
}

const char* X::getResName(const char* name) {
	static char ret[FS_FULL_NAME_MAX];
	const char* wkdir = x_get_work_dir();
	if(wkdir[1] == 0 && wkdir[0] == '/')
		snprintf(ret, FS_FULL_NAME_MAX-1, "/res/%s", name);
	else
		snprintf(ret, FS_FULL_NAME_MAX-1, "%s/res/%s", wkdir, name);
	return ret;
}

bool X::getDesktopSpace(grect_t& r, int index) {
	if(x_get_desktop_space(index, &r) == 0)
		return true;
	return false;
}

bool X::setDesktopSpace(const grect_t& r, int index) {
	if(x_set_desktop_space(index, &r) == 0)
		return true;
	return false;
}