#ifndef ILI9486_H
#define ILI9486_H

#include <stdint.h>

#define LCD_WIDTH	320
#define LCD_HEIGHT	240

void lcd_init(int pin_rs, int pin_cs, int pin_rst, int cdiv);
void lcd_flush(const void* buf, uint32_t size);

#endif
