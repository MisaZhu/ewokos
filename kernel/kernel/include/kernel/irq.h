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

/*
 * Per-core cascade guard shared with the arch fault dispatchers: an arch
 * handler that dumps the dying proc and calls proc_exit directly (e.g. the
 * aarch64 sync-exception generic path) wraps that tail with enter/leave so a
 * fault re-triggered by the corrupt proc halts instead of recursing. Must NOT
 * be entered around a call into data/prefetch_abort_handler - those take the
 * guard themselves and a second enter would look like a nested abort.
 */
extern uint32_t abort_guard_enter(const char* what);
extern void     abort_guard_leave(uint32_t core);

/*
 * Non-zero if the saved context faulted while the CPU was in KERNEL mode.
 * Used by the abort handlers to halt-with-dump on a kernel bug instead of
 * killing an innocent user proc (and then schedule()) on a misattributed fault.
 */
extern int abort_from_kernel(context_t* ctx);

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
