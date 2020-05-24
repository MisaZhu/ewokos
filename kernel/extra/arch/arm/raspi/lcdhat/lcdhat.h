#ifndef LCDHAT_H
#define LCDHAT_H

#include <graph.h>

#define LCD_HEIGHT 240
#define LCD_WIDTH 240

void lcd_init(void);
void lcd_flush(graph_t* g);

#endif
