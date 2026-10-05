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

/*
 * Arch hooks that let the common kernel/kernel/src/irq.c stay free of
 * architecture #if macros. Each is implemented in the per-platform
 * kernel/platform/<arch>/arch/common/src/irq.c. See that file for the exact
 * per-arch behaviour these reproduce.
 */
#include <kevent.h>

/* raw interrupt vector for this trap: x86 reads it from the trap frame, the
   other arches ask the controller through irq_get_arch(). */
extern uint32_t arch_irq_raw(context_t* ctx);
/* acknowledge/clear the timer source after a timer0 tick (no-op on x86, which
   accounts the PIT tick in arch_irq_prologue instead). */
extern void arch_irq_timer_ack(void);
/* per-arch work done at the very top of irq_handler() before accounting the
   current proc (x86 bumps the PIT software tick; others no-op). */
extern void arch_irq_prologue(context_t* ctx);
/* idle CPU pause in the irq tail (wfi on arm/aarch64/riscv; no-op on x86). */
extern void arch_irq_idle(void);
/*
 * Permit userspace to read the fine-grained free-running counter that libc
 * interpolates the vsyscall clock from (CNTKCTL_EL1.EL0VCTEN on aarch64,
 * CNTKCTL.PL0VCTEN on armv7, scounteren.TM on riscv, no-op on x86 and on arm
 * SoCs without a generic timer). These are per-core control registers, so this
 * must run on every core before any user task is dispatched there - the common
 * kernel calls it from irq_init() for the boot core and from
 * _slave_kernel_entry_c() for each AP.
 */
extern void arch_enable_user_cnt(void);
/* whether a data-abort status describes a recoverable user page fault. */
extern uint8_t arch_fault_recoverable(uint32_t status);
/* snapshot the arch register file into a core-dump event. */
extern void arch_fill_core_dump_regs(kev_core_dump_t* dump, context_t* ctx);
/* format+print the user words around a faulting address (arch word layout). */
extern void arch_dump_addr_words(const uint8_t* page_ptr, uint32_t page_off,
        uint32_t avail, const char* tag);
/* dump the dying user proc's stack/pc/lr words; no-op on arches that never
   enabled the user-word dump. */
extern void arch_dump_user_fault(proc_t* proc, context_t* ctx);
/* arch-specific extra line printed by prefetch_abort_handler (x86 prints the
   live trap-frame summary; others no-op). */
extern void arch_dump_prefetch_extra(context_t* ctx);

#endif
