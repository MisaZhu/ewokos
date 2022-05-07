#ifndef ILI9486_H
#define ILI9486_H

#include <stdint.h>

extern uint16_t LCD_HEIGHT;
extern uint16_t LCD_WIDTH;

/*LCD_RS	Instruction/Data Register selection
  LCD_CS    LCD chip selection, low active
  LCD_RST   LCD reset
  */
void ili9486_init(int pin_rs, int pin_cs, int pin_rst, int cdiv);
void ili9486_flush(const void* buf, uint32_t size);

#endif
