#ifndef UART_H
#define UART_H

void uartInit(void);
void uartTransmit(char c);
bool uartReadyToRecv(void);
int uartReceive(void);

#endif
