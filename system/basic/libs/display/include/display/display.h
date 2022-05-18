#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stdint.h>

enum {
    DISP_GET_DISP_DEV = 0,
    DISP_ADD_DISP_DEV,
    DISP_GET_DISP_NUM
};

#define DISP_MAX  4

const char* get_display_fb_dev(const char* display_man_dev, uint32_t display_index);
uint32_t    add_display_fb_dev(const char* display_man_dev, const char* fb_dev);
uint32_t    get_display_num(const char* display_man_dev);

#endif