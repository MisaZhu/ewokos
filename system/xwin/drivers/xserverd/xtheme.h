#ifndef X_DEV_THEME_H
#define X_DEV_THEME_H

#include <xserver.h>
#include <x/xtheme.h>

int x_load_theme(x_t* x, const char* name, x_theme_t* theme);
int x_load_xwm_theme(x_t* x, const char* name, xwm_theme_t* theme);

#endif