#ifndef SYS_INTR_H
#define SYS_INTR_H

#include <stdint.h>
#include <interrupt.h>

typedef void(*interrupt_handler_t)(uint32_t interrupt);

int32_t sys_interrupt_setup(uint32_t interrupt, interrupt_handler_t handler);

void sys_interrupt_end(void);


#endif
