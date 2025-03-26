#ifndef X_DEV_THEME_H
#define X_DEV_THEME_H

#include <x/xtheme.h>

int x_load_theme(const char* name, x_theme_t* theme);
int x_load_xwm_theme(const char* name, xwm_theme_t* theme);

#endif