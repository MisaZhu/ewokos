#ifndef UART_H
#define UART_H

#include <types.h>

void uart_init(void);
void uart_handle(void);
void uart_putch(int c);
int32_t uart_getch();

#endif
