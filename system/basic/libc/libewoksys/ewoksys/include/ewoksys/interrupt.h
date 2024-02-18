#ifndef SYS_INTR_H
#define SYS_INTR_H

#include <stdint.h>
#include <interrupt.h>

typedef void(*interrupt_handler_t)(uint32_t interrupt, uint32_t data);

int32_t sys_interrupt_setup(uint32_t interrupt, uint32_t irq_raw, interrupt_handler_t handler, uint32_t data);


#endif
