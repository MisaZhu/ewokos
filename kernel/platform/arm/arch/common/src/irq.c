#include <kernel/irq.h>
#include <kprintf.h>
#include <kernel/system.h>
#include <dev/timer.h>
#include <stdint.h>

void dump_ctx(context_t* ctx) {
    printf("ctx dump:\n"
        "  cpsr=0x%x\n"
        "  pc=0x%x\n"
        "  sp=0x%x\n"
        "  lr=0x%x\n",
        ctx->cpsr,
        ctx->pc,
        ctx->sp,
        ctx->lr);

    uint32_t i;
    for(i=0; i<13; i++) 
        printf("  r%d: 0x%x\n", i, ctx->gpr[i]);
}

/* ---- arch hooks declared in <kernel/irq.h> (see common kernel/src/irq.c) ---- */

uint32_t arch_irq_raw(context_t* ctx) {
    (void)ctx;
    return irq_get_arch();
}

void arch_irq_timer_ack(void) {
    timer_clear_interrupt(0);
}

void arch_irq_prologue(context_t* ctx) {
    (void)ctx;
}

void arch_irq_idle(void) {
    wfi();
}

uint8_t arch_fault_recoverable(uint32_t status) {
    return (uint8_t)(((status & 0x5) == 0x5) || ((status & 0xD) == 0xD));
}

int abort_from_kernel(context_t* ctx) {
    if(ctx == NULL)
        return 1; /* unknown context: treat as kernel, never kill a proc on a guess */
    /* CPSR M[4:0]: 0b10000 (0x10) = USR; SVC/SYS/ABT/IRQ/FIQ are kernel modes */
    return (ctx->cpsr & 0x1F) != 0x10;
}

void arch_fill_core_dump_regs(kev_core_dump_t* dump, context_t* ctx) {
    dump->regs.arm.cpsr = ctx->cpsr;
    dump->regs.arm.pc = ctx->pc;
    dump->regs.arm.sp = ctx->sp;
    dump->regs.arm.lr = ctx->lr;
    for(int i=0; i<13; i++)
        dump->regs.arm.gpr[i] = ctx->gpr[i];
}

void arch_dump_addr_words(const uint8_t* page_ptr, uint32_t page_off,
        uint32_t avail, const char* tag) {
    if(avail >= sizeof(uint64_t)) {
        uint32_t i;
        uint32_t count = avail / sizeof(uint64_t);
        if(count > 6) {
            count = 6;
        }
        printf("%s64:", tag);
        for(i = 0; i < count; ++i) {
            uint64_t val = ((const uint64_t*)(page_ptr + page_off))[i];
            printf(" %08x%08x",
                    (uint32_t)(val >> 32),
                    (uint32_t)val);
        }
        printf("\n");
    }
    if(avail >= sizeof(uint32_t)) {
        uint32_t i;
        uint32_t count32 = avail / sizeof(uint32_t);
        if(count32 > 8) {
            count32 = 8;
        }
        printf("%s32:", tag);
        for(i = 0; i < count32; ++i) {
            uint32_t val = ((const uint32_t*)(page_ptr + page_off))[i];
            printf(" %08x", val);
        }
        printf("\n");
    }
}

void arch_dump_user_fault(proc_t* proc, context_t* ctx) {
    /* legacy ARM builds never enabled the user-word dump */
    (void)proc;
    (void)ctx;
}

void arch_dump_prefetch_extra(context_t* ctx) {
    (void)ctx;
}
#ifdef ARM_V7
/*
 * <dev/timer.h>: raw ARM generic virtual counter behind
 * timer_read_sys_usec(), published to userspace so libc can interpolate the
 * vsyscall clock between scheduler ticks. Same CNTVCT/CNTFRQ pair the v7 BSPs
 * read, so kernel usec and libc nsec share one source.
 */
__attribute__((weak)) uint32_t timer_fine_cnt(uint64_t* cnt) {
    uint32_t frq;
    __asm__ volatile("mrc p15, 0, %0, c14, c0, 0" : "=r" (frq) ); /* CNTFRQ */
    if(frq == 0)
        return 0;
    if(cnt != NULL) {
        uint64_t v;
        __asm__ volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (v)); /* CNTVCT */
        *cnt = v;
    }
    return frq;
}

/*
 * <kernel/irq.h>: CNTKCTL resets UNKNOWN, so PL0 reads of CNTVCT/CNTPCT take
 * an undefined-instruction abort until they are enabled. Per-core register.
 *
 * Read-modify-write, never a blind store: CNTKCTL also carries the event-stream
 * controls (EVNTEN/EVNTDIR/EVNTI), and a kernel spinlock that parks on `wfe`
 * relies on the periodic event to recover from a missed `sev`. Overwriting the
 * whole register with 0x3 disables that stream and deadlocks the lock - the same
 * trap the aarch64 port fell into. Only bits [1:0] (PL0PCTEN|PL0VCTEN) are
 * touched here.
 */
void arch_enable_user_cnt(void) {
    uint32_t val;
    __asm__ volatile("mrc p15, 0, %0, c14, c1, 0" : "=r" (val)); /* CNTKCTL */
    val |= 0x3; /* PL0PCTEN | PL0VCTEN */
    __asm__ volatile("mcr p15, 0, %0, c14, c1, 0" :: "r"(val));
    __asm__ volatile("isb" ::: "memory");
}
#else
/*
 * Pre-ARMv7 targets in this tree have no generic timer to expose: lego.ev3's
 * AM1808 (ARM926EJ-S) times off a Davinci TIM34 MMIO counter, which cannot be
 * handed to userspace without mapping device registers into every address
 * space. Publish hz == 0 so libc keeps the tick-quantized clock - and, more
 * importantly, never emits the mrrc above, which would assemble but abort at
 * runtime on a core that has no CP14 timer.
 */
__attribute__((weak)) uint32_t timer_fine_cnt(uint64_t* cnt) {
    (void)cnt;
    return 0;
}

void arch_enable_user_cnt(void) {
    /* no user-readable counter on this platform */
}
#endif
