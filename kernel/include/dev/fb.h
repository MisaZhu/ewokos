#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <fbinfo.h>

extern char _fbStart[];

bool fbInit();

FBInfoT* fbGetInfo();

#endif
