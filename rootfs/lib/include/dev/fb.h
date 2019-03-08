#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <fbinfo.h>

int fbGetInfo(FBInfoT* info);

int fbOpen();

int fbClose();

int fbWrite(void* data, uint32_t size);

#endif
