
#ifndef __UTILS_H__
#define __UTILS_H__

#include <types.h>

int to_hex(const char *str, uint8_t* hex, int len);
int to_str(char *str, uint8_t* hex, size_t len);

#endif