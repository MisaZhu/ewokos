#ifndef XPT2046_H
#define XPT2046_H

#include <stdint.h>

int xpt2046_read(uint16_t* press,  uint16_t* x, uint16_t* y);
void xpt2046_init(int pin_cs, int pin_irq, int cdiv);

#endif
