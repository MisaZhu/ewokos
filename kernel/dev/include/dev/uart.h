#ifndef UART_H
#define UART_H

#include <stdint.h>

int32_t uart_dev_init(uint32_t baud);
int32_t uart_write(const void* data, uint32_t size);

#endif
