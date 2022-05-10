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

#define DISP_MAX  4

const char* get_disp_fb_dev(const char* disp_man_dev, uint32_t disp_index);
bool        is_disp_top(const char* disp_man_dev, uint32_t disp_index);
bool        set_disp_top(const char* disp_man_dev, uint32_t disp_index);
uint32_t    get_disp_num(const char* disp_man_dev);

#endif