#ifndef RGB15_H
#define RGB15_H

#include <stdint.h>

void argb_2_rgb15(uint16_t *out, uint32_t *in , int w, int h);
void rgb15_2_argb(uint32_t *out, uint16_t *in , int w, int h);

/* Big-endian XRGB1555 byte stream -> host-order ARGB8888.
 * Reads raw bytes; no pre-swap needed. */
void rgb15be_2_argb(uint32_t *out, const uint8_t *in, int bpr, int w, int h);

#endif
