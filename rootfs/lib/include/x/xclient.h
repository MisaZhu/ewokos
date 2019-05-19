#ifndef XCLIENT_H
#define XCLIENT_H

#include <x/xcmd.h>

int32_t x_open(const char* dev);
void x_close(int32_t fd);

#endif
