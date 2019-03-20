#ifndef BASE16_H
#define BASE16_H

#include "types.h"

void base16_encode(const char *src, i32_t srcLen, char *dst, i32_t *dstLen);
void base16_decode(const char *src, i32_t srcLen, char *dst, i32_t *dstLen);

#endif
