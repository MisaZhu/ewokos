#ifndef ILI9486_H
#define ILI9486_H

#include <stdint.h>

extern uint16_t LCD_HEIGHT;
extern uint16_t LCD_WIDTH;

void ili9486_init(int pin_dc, int pin_cs, int pin_rst);
void ili9486_flush(const void* buf, uint32_t size);

#endif
