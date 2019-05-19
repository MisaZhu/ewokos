#ifndef XCLIENT_H
#define XCLIENT_H

#include <x/xcmd.h>

int32_t x_open(const char* dev);

void x_close(int32_t fd);

void x_move_to(int32_t fd, int32_t x, int32_t y);

void x_resize_to(int32_t fd, int32_t w, int32_t h);

void x_set_bg(int32_t fd, uint32_t color);

void x_set_color(int32_t fd, uint32_t color);

void x_set_font(int32_t fd, const char* name, uint32_t size, uint32_t style);

void x_pixel(int32_t fd, int32_t x, int32_t y);

void x_clear(int32_t fd);

void x_box(int32_t fd, int32_t x, int32_t y, int32_t w, int32_t h);

void x_fill(int32_t fd, int32_t x, int32_t y, int32_t w, int32_t h);

void x_line(int32_t fd, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

void x_text(int32_t fd, int32_t x, int32_t y, const char* str);

#endif
