#ifndef PRINTK_H
#define PRINTK_H

#include <stdint.h>

void kout(const char* s, uint32_t len);
void kout_str(const char* s);
void printf(const char *format, ...);

#endif
