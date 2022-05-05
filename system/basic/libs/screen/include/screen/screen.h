#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>

enum {
    SCR_SET_TOP = 0,
    SCR_GET_TOP,
    SCR_GET_FBDEV
};

const char* get_scr_fb_dev(const char* scr_dev);
bool        is_scr_top(const char* scr_dev);

#endif