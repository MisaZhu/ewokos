#ifndef UART_H
#define UART_H

#include <types.h>

void uartDevInit(void); //arch init

void uartInit(void);
void uartTransmit(char c);
bool uartReadyToRecv(void);
int uartReceive(void);

void uartPuts(const char* str);

void uartPutch(int c);
int uartGetch();

#endif
