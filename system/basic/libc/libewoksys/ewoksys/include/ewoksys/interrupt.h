#ifndef SYS_INTR_H
#define SYS_INTR_H

#include <stdint.h>
#include <interrupt.h>

typedef void(*interrupt_handler_func_t)(uint32_t interrupt, uint32_t data);

typedef struct {
	interrupt_handler_func_t handler;
	uint32_t data;
} interrupt_handler_t;

int32_t sys_interrupt_setup(uint32_t irq, interrupt_handler_t* handler);


#endif
