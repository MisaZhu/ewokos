#ifndef BASE16_H
#define BASE16_H

#include <stdint.h>

void b16_encode(const char *input, uint32_t input_len, char *output, uint32_t *output_len);
void b16_decode(const char *input, uint32_t input_len, char *output, uint32_t *output_len);

#endif
