#ifndef UART_H
#define UART_H

#include <dev/kdevice.h>

int32_t uart_dev_init(void);
int32_t uart_write(dev_t* dev, const void* data, uint32_t size);

#endif
