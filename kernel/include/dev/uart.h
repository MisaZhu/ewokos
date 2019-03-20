#ifndef UART_H
#define UART_H

#include <types.h>

void uart_dev_init(void); //arch init

void uart_init(void);
void uart_trans(char c);
bool uart_ready_to_recv(void);
int uart_recv(void);

void uart_puts(const char* str);

void uart_putch(int c);
int uart_getch();

#endif
