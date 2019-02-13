#ifndef BASE16_H
#define BASE16_H

#include "types.h"

void base16Encode(const char *src, i32_t srcLen, char *dst, i32_t *dstLen);
void base16Decode(const char *src, i32_t srcLen, char *dst, i32_t *dstLen);

#endif
