#ifndef IRQ_H
#define IRQ_H

#include <kernel/context.h>
#include <kernel/proc.h>
#include <interrupt.h>

extern void dump_ctx(context_t *ctx);
extern void dump_user_addr_words(proc_t* proc, ewokos_addr_t addr, const char* tag);
extern void dump_user_fault_words(proc_t* proc, context_t* ctx);
extern void irq_handler(context_t* ctx);
extern void undef_abort_handler(context_t* ctx, uint32_t status);
extern void prefetch_abort_handler(context_t* ctx, uint32_t status);
extern void data_abort_handler(context_t* ctx, ewokos_addr_t addr_fault, uint32_t status);
extern void irq_init(void);

extern void irq_init_arch(void);
extern void irq_enable_arch(uint32_t irq);
extern void irq_enable_core_arch(uint32_t core, uint32_t irq);
extern void irq_clear_arch(uint32_t irq);
extern void irq_clear_core_arch(uint32_t core, uint32_t irq);
extern void irq_disable_arch(uint32_t irq);

extern uint32_t irq_get_arch(void);
extern uint32_t irq_get_unified_arch(uint32_t irq_raw);
extern void irq_eoi_arch(uint32_t irq_raw);
extern uint64_t irq_accounting_now_usec(void);

#endif
