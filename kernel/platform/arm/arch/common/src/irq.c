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