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
extern void irq_enable_arch(uint32_t irq);
extern void irq_enable_core_arch(uint32_t core, uint32_t irq);
extern void irq_clear_arch(uint32_t irq);
extern void irq_clear_core_arch(uint32_t core, uint32_t irq);
extern void irq_disable_arch(uint32_t irq);

extern uint32_t irq_get_arch(void);
extern uint32_t irq_get_unified_arch(uint32_t irq_raw);
extern void irq_eoi_arch(uint32_t irq_raw);

#endif
