#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stdint.h>

enum {
    DISP_SET_TOP = 0,
    DISP_GET_TOP,
    DISP_GET_DISP_NUM,
    DISP_GET_DISP_DEV
};

const char* get_disp_fb_dev(const char* disp_dev, uint32_t disp_index);
bool        is_disp_top(const char* disp_dev, uint32_t disp_index);
bool        set_disp_top(const char* disp_dev, uint32_t disp_index);

#endif