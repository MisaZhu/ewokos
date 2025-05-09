#ifndef HW_ARCH_H
#define HW_ARCH_H

#include <stdint.h>

enum {
	UART_MINI = 0,
	UART_PL011
};

extern uint32_t _uart_type;
extern uint32_t _pi4;

#endif
