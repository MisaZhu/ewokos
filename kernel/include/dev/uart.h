#ifndef UART_H
#define UART_H

#include <types.h>

void uartDevInit(void); //arch init

void uartInit(void);
void uartTransmit(char c);
bool uartReadyToRecv(void);
int uartReceive(void);

void kputch(int c);
void kputs(const char* str);

#endif
