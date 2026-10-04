#include <kernel/irq.h>
#include <kernel/svc.h>
#include <kernel/system.h>
#include <kernel/proc.h>
#include <kernel/core.h>
#include <kprintf.h>
#include <kstring.h>
#include <arch_context.h>
#include <x86_smp.h>
#include <dev/timer.h>
#include <stddef.h>
#include <stdint.h>

void dump_ctx(context_t* ctx) {
    printf("\nctx=%08x\n", (uint32_t)(uintptr_t)ctx);
    printf("cr2=%08x trap=%08x err=%08x\n", (uint32_t)ctx->cr2, (uint32_t)ctx->trap_no, (uint32_t)ctx->err_code);
    printf("pc=%08x lr=%08x sp=%08x cs=%08x ss=%08x flags=%08x\n",
        (uint32_t)ctx->pc,
        (uint32_t)ctx->lr,
        (uint32_t)ctx->sp,
        (uint32_t)ctx->cs,
        (uint32_t)ctx->ss,
        (uint32_t)ctx->rflags);
    printf("rdi=%08x rsi=%08x rdx=%08x rcx=%08x\n",
        (uint32_t)ctx->gpr[0],
        (uint32_t)ctx->gpr[1],
        (uint32_t)ctx->gpr[2],
        (uint32_t)ctx->gpr[3]);
    printf("r8 =%08x r9 =%08x rax=%08x rbx=%08x rbp=%08x\n",
        (uint32_t)ctx->gpr[4],
        (uint32_t)ctx->gpr[5],
        (uint32_t)ctx->gpr[6],
        (uint32_t)ctx->gpr[7],
        (uint32_t)ctx->gpr[8]);
    printf("r10=%08x r11=%08x r12=%08x r13=%08x r14=%08x r15=%08x\n",
        (uint32_t)ctx->gpr[9],
        (uint32_t)ctx->gpr[10],
        (uint32_t)ctx->gpr[11],
        (uint32_t)ctx->gpr[12],
        (uint32_t)ctx->gpr[13],
        (uint32_t)ctx->gpr[14]);
}

static void trap_panic(uint64_t vector, context_t* ctx) {
    proc_t* cproc = get_current_proc();
    if (cproc != NULL) {
        printf("pid:%d cmd:%s trap:%d\n", cproc->info.pid, cproc->info.cmd, (int)vector);
    }
    else {
        printf("kernel trap:%d\n", (int)vector);
    }
    dump_ctx(ctx);
    halt();
}

void handle_trap(uint64_t vector, uint64_t err, context_t* ctx) {
    ctx->trap_no = vector;
    ctx->err_code = err;
    ctx->lr = ctx->pc;

    if ((vector >= 0x20 && vector < 0x30) || vector == X86_VECTOR_IPI) {
        irq_handler(ctx);
        return;
    }

    if (vector == 0x80) {
        svc_handler((int32_t)ctx->gpr[0],
            (ewokos_addr_t)ctx->gpr[1],
            (ewokos_addr_t)ctx->gpr[2],
            (ewokos_addr_t)ctx->gpr[3],
            ctx);
        return;
    }

    if (vector == 14) {
        data_abort_handler(ctx, (ewokos_addr_t)ctx->cr2, (uint32_t)err);
        return;
    }

    if (vector == 13) {
        prefetch_abort_handler(ctx, (uint32_t)err);
        return;
    }

    if (vector == 6 || vector == 3 || vector == 1) {
        undef_abort_handler(ctx, (uint32_t)vector);
        return;
    }

    trap_panic(vector, ctx);
}

/* ---- arch hooks declared in <kernel/irq.h> (see common kernel/src/irq.c) ---- */

uint32_t arch_irq_raw(context_t* ctx) {
    /* x86 trap entry already records the raw vector in the trap frame. */
    return (uint32_t)ctx->trap_no;
}

void arch_irq_timer_ack(void) {
    /* x86 accounts the PIT tick in arch_irq_prologue instead. */
}

void arch_irq_prologue(context_t* ctx) {
    if(get_core_id() == 0 && irq_get_unified_arch((uint32_t)ctx->trap_no) == IRQ_TIMER0) {
        /*
         * x86 PIT time only advances when timer_clear_interrupt() bumps the
         * software tick counter. Account that tick before pausing the current
         * task, otherwise core0 loses almost the entire last slice and shows up
         * as fake 100% kernel residual.
         */
        timer_clear_interrupt(0);
    }
}

void arch_irq_idle(void) {
    /* x86 idle path does not issue a halt here. */
}

uint8_t arch_fault_recoverable(uint32_t status) {
    return (uint8_t)(((status & 0x5) == 0x5) || ((status & 0xD) == 0xD));
}

int abort_from_kernel(context_t* ctx) {
    if(ctx == NULL)
        return 1; /* unknown context: treat as kernel, never kill a proc on a guess */
    /* CS RPL: 0 = kernel (X86_KERNEL_CS 0x08), 3 = user (X86_USER_CS 0x23) */
    return (ctx->cs & 0x3) == 0;
}

void arch_fill_core_dump_regs(kev_core_dump_t* dump, context_t* ctx) {
    dump->regs.x86.cr2 = ctx->cr2;
    dump->regs.x86.trap_no = ctx->trap_no;
    dump->regs.x86.err_code = ctx->err_code;
    dump->regs.x86.pc = ctx->pc;
    dump->regs.x86.lr = ctx->lr;
    dump->regs.x86.cs = ctx->cs;
    dump->regs.x86.rflags = ctx->rflags;
    dump->regs.x86.sp = ctx->sp;
    dump->regs.x86.ss = ctx->ss;
    for(int i=0; i<15; i++)
        dump->regs.x86.gpr[i] = ctx->gpr[i];
}

void arch_dump_addr_words(const uint8_t* page_ptr, uint32_t page_off,
        uint32_t avail, const char* tag) {
    ewokos_addr_t words[4];
    memset(words, 0, sizeof(words));
    memcpy(words, page_ptr + page_off, avail < sizeof(words) ? avail : sizeof(words));
    printf("%s: %08x %08x %08x %08x\n",
            tag,
            (uint32_t)words[0],
            (uint32_t)words[1],
            (uint32_t)words[2],
            (uint32_t)words[3]);
}

void arch_dump_user_fault(proc_t* proc, context_t* ctx) {
    dump_user_fault_words(proc, ctx);
}

void arch_dump_prefetch_extra(context_t* ctx) {
    printf("live: pc=%x sp=%x cs=%x ss=%x trap=%x err=%x\n",
            (uint32_t)ctx->pc,
            (uint32_t)ctx->sp,
            (uint32_t)ctx->cs,
            (uint32_t)ctx->ss,
            (uint32_t)ctx->trap_no,
            (uint32_t)ctx->err_code);
}

/*
 * <dev/timer.h>: the x86 tick comes from the PIT, an IO-port device with no
 * userspace-visible counter. rdtsc does supply a free-running counter, but only a
 * board that has CALIBRATED its frequency can convert it to nanoseconds - an
 * uncalibrated TSC would produce plausible-looking but meaningless timestamps.
 * This weak default therefore publishes hz == 0 (tick-quantized clock); a BSP that
 * calibrates the TSC (see machines/x86/kernel/bsp/timer.c) overrides this symbol
 * with the measured frequency and its own rdtsc read.
 */
__attribute__((weak)) uint32_t timer_fine_cnt(uint64_t* cnt) {
    (void)cnt;
    return 0;
}

/*
 * <kernel/irq.h>: a userland rdtsc takes a #GP while CR4.TSD (bit 2) is set. TSD
 * resets clear and this kernel never sets it, but clear it explicitly so the
 * userspace counter libc interpolates from is guaranteed not to fault. CR4 is
 * per-core, and the common kernel calls this on the boot core and on every AP -
 * mirroring the CNTKCTL/scounteren enable on arm/aarch64/riscv. Harmless on a BSP
 * that publishes no counter (fine_cnt_hz == 0 gates the userspace read off).
 */
void arch_enable_user_cnt(void) {
    uint64_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    if(cr4 & (1ULL << 2)) {          /* TSD */
        cr4 &= ~(1ULL << 2);
        __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
    }
}
