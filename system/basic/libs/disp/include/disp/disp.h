#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stdint.h>

enum {
    DISP_GET_DISP_DEV = 0,
    DISP_GET_DISP_NUM
};

#define DISP_MAX  4

const char* get_disp_fb_dev(const char* disp_man_dev, uint32_t disp_index);
uint32_t    get_disp_num(const char* disp_man_dev);

#endif