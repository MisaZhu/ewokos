#ifndef IRQ_H
#define IRQ_H

#include <kernel/context.h>
#include <interrupt.h>

extern void dump_ctx(context_t *ctx);
extern void irq_handler(context_t* ctx);
extern void prefetch_abort_handler(context_t* ctx, uint32_t status);
extern void data_abort_handler(context_t* ctx, ewokos_addr_t addr_fault, uint32_t status);
extern void irq_init(void);
extern void irq_arch_init(void);
extern void irq_enable(uint32_t irq);
extern void irq_disable(uint32_t irq);
extern void irq_enable_cpsr(context_t* ctx);
extern void irq_disable_cpsr(context_t* ctx);
extern uint32_t irq_get(void);

#endif
