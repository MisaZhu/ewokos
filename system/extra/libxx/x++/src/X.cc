#include "x++/X.h"
#include "x++/XWin.h"
#include <stdio.h>

bool X::open(XWin* xwin, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style) {
	xwin_t* xw = x_open(&this->x, x, y, w, h, title, style);
	if(xw == NULL)
		return false;
	xw->data = xwin;
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

bool X::screenInfo(xscreen_t& scr) {
	return (x_screen_info(&scr) == 0);
}

