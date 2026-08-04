#ifndef SYS_INTR_H
#define SYS_INTR_H

#include <ewoksys/ewokdef.h>
#include <interrupt.h>

typedef void(*interrupt_handler_func_t)(uint32_t interrupt, ewokos_addr_t data);

typedef struct {
	interrupt_handler_func_t handler;
        ewokos_addr_t data;
} interrupt_handler_t;

int32_t sys_interrupt_setup(uint32_t irq, interrupt_handler_t* handler);

int32_t sys_soft_intr(int32_t pid, ewokos_addr_t entry, ewokos_addr_t data);

#endif
