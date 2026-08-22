#ifndef RGB24_H
#define RGB24_H

#include <stdint.h>

void argb_2_rgb24(uint32_t *out,  uint32_t *in , int w, int h);
void rgb24_2_argb(uint32_t *out,  uint32_t *in , int w, int h);

/* Big-endian [00][RR][GG][BB] byte stream -> host-order ARGB8888.
 * Reads raw bytes; no pre-swap needed. */
void rgb24be_2_argb(uint32_t *out, const uint8_t *in, int bpr, int w, int h);

#endif
