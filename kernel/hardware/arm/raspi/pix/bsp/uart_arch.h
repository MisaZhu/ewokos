#ifndef UART_ARCH_H
#define UART_ARCH_H

#include <stdint.h>

enum {
	UART_MINI = 0,
	UART_PL011
};

extern uint32_t _uart_type;

#endif
