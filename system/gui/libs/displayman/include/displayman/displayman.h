#ifndef DISPLAYMAN_H
#define DISPLAYMAN_H

#include <stdbool.h>
#include <stdint.h>
#include <display/display.h>

enum {
    DISP_GET_DISP_DEV = 0,
    DISP_ADD_DISP_DEV,
    DISP_GET_DISP_NUM
};

#define DISP_MAX  8

const char* displayman_get_dev(const char* display_man_dev, uint32_t display_index);
int32_t     displayman_add_dev(const char* display_man_dev, const char* dev_name, uint32_t display_index);
uint32_t    displayman_get_num(const char* display_man_dev);
int         displayman_open(const char* display_man_dev, uint32_t display_index, display_t* display);


#endif
