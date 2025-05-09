#ifndef PL011_UART_H
#define PL011_UART_H

#include <stdint.h>

int32_t bcm283x_pl011_uart_init(void);
int32_t bcm283x_pl011_uart_write(const void* data, uint32_t size);
int32_t bcm283x_pl011_uart_ready_to_recv(void);
int32_t bcm283x_pl011_uart_recv(void);

#endif
