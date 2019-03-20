#ifndef IRQ_H
#define IRQ_H

#include <types.h>

/* number of interrupt lines */
#define IRQ_COUNT 32

typedef void (*interrupt_handler_t)(void);

extern void register_interrupt_handler(uint32_t line, interrupt_handler_t handler);
extern void handle_irq(void);

/* architecture specific functions */
extern void enable_irq(uint32_t line);
extern void get_pending_irqs(bool *result);
extern void irq_init();

#endif
