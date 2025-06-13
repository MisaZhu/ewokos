#include "x++/X.h"
#include "x++/XWin.h"
#include <stdio.h>
#include <ewoksys/basic_math.h>
#include <font/font.h>

using namespace Ewok;

static font_t* _sysFont = NULL;

X::X(void) {
	x_init(&x, this);
}

int X::getTheme(x_theme_t* theme) {
	return x_get_theme(theme);
}

font_t* X::getSysFont(void) {
	if(_sysFont == NULL)
		_sysFont = font_new(DEFAULT_SYSTEM_FONT, true);
	return _sysFont;
}

void X::run(void (*loop)(void*), void* p) {
	x.on_loop = loop;
	x_run(&x, p);
}

void X::terminate(void) {
	x_terminate(&x);
}

bool X::terminated(void) {
	return x.terminated;
}

bool X::getScreenInfo(xscreen_info_t& scr, int index) {
	return (x_screen_info(&scr, index) == 0);
}

uint32_t X::getDisplayNum() {
	return x_get_display_num();
}

const char* X::getResName(const char* name) {
	return x_get_res_name(name);
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