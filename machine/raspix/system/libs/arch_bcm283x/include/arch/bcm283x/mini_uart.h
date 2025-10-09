#ifndef MINI_UART_H
#define MINI_UART_H

#include <stdint.h>

int32_t bcm283x_mini_uart_init(void);
int32_t bcm283x_mini_uart_write(const void* data, uint32_t size);
int32_t bcm283x_mini_uart_recv(void);
int32_t bcm283x_mini_uart_send(uint8_t data);

#endif
