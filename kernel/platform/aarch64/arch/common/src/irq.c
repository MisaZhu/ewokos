#include <kernel/irq.h>
#include <kprintf.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <dev/timer.h>
#include <stdint.h>

#define SPLIT(x)	((uint32_t)(((uint64_t)x)>>32)),((uint32_t)(x))

static const char* esr_ec_name(uint32_t ec) {
    switch(ec) {
    case 0x00: return "unknown";
    case 0x01: return "wfi/wfe";
    case 0x03: return "mcr/mrc cp15";
    case 0x04: return "mcrr/mrrc cp15";
    case 0x05: return "mcr/mrc cp14";
    case 0x06: return "ldc/stc cp14";
    case 0x07: return "sve/simd/fp trap";
    case 0x0E: return "illegal execution";
    case 0x11: return "svc aarch32";
    case 0x15: return "svc aarch64";
    case 0x18: return "msr/mrs/sys";
    case 0x20: return "instr abort lower el";
    case 0x21: return "instr abort same el";
    case 0x22: return "pc alignment";
    case 0x24: return "data abort lower el";
    case 0x25: return "data abort same el";
    case 0x26: return "sp alignment";
    case 0x2C: return "fp exception aarch64";
    case 0x2F: return "serror";
    case 0x30: return "breakpoint lower el";
    case 0x31: return "breakpoint same el";
    case 0x32: return "software step lower el";
    case 0x33: return "software step same el";
    case 0x34: return "watchpoint lower el";
    case 0x35: return "watchpoint same el";
    case 0x3C: return "brk aarch64";
    default:   return "reserved";
    }
}

static const char* abort_fsc_name(uint32_t fsc) {
    switch(fsc & 0x3F) {
    case 0x00: return "addr size fault level0";
    case 0x01: return "addr size fault level1";
    case 0x02: return "addr size fault level2";
    case 0x03: return "addr size fault level3";
    case 0x04: return "translation fault level0";
    case 0x05: return "translation fault level1";
    case 0x06: return "translation fault level2";
    case 0x07: return "translation fault level3";
    case 0x09: return "access flag fault level1";
    case 0x0A: return "access flag fault level2";
    case 0x0B: return "access flag fault level3";
    case 0x0D: return "permission fault level1";
    case 0x0E: return "permission fault level2";
    case 0x0F: return "permission fault level3";
    case 0x10: return "sync external abort";
    case 0x11: return "sync tag check fault";
    case 0x14: return "sync external abort level0";
    case 0x15: return "sync external abort level1";
    case 0x16: return "sync external abort level2";
    case 0x17: return "sync external abort level3";
    case 0x18: return "sync parity/ecc";
    case 0x1C: return "sync parity/ecc level0";
    case 0x1D: return "sync parity/ecc level1";
    case 0x1E: return "sync parity/ecc level2";
    case 0x1F: return "sync parity/ecc level3";
    case 0x21: return "alignment fault";
    case 0x22: return "debug event";
    default:   return "other/impl defined";
    }
}

void dump_ctx(context_t* ctx) {
    uint64_t sp, esr, far;
    __asm__ __volatile__("mov %0, sp":"=r" (sp) :  : "memory");
    __asm__ __volatile__("mrs %0, esr_el1":"=r" (esr) :  : "memory");
    __asm__ __volatile__("mrs %0, far_el1":"=r" (far) :  : "memory");
    printf("\nESR:%08x%08x\n", SPLIT(esr));
    printf("FAR:%08x%08x\n", SPLIT(far));
    printf("KSP:%08x%08x\n", SPLIT(sp));
    printf("CTX:%08x%08x\n"
        "pc : %08x%08x\t"
        "spsr:%08x%08x\t"
        "sp : %08x%08x\t"
        "lr : %08x%08x\n",
        SPLIT(ctx),
        SPLIT(ctx->pc),
        SPLIT(ctx->spsr_el1),
        SPLIT(ctx->sp),
        SPLIT(ctx->lr)
    );

    uint32_t i;
    for(i=0; i<30; i++){
        if(i > 0 && i % 4 == 0 )
            printf("\n");
        printf("x%02d: %08x%08x\t", i, (uint32_t)((ctx->gpr[i]>>32)), (uint32_t)ctx->gpr[i]);
    }
    printf("\n");
    //dump_page_tables(_kernel_info.kernel_vm);
}

void sync_exception_handle(uint64_t esr, uint64_t far, context_t* ctx){
    proc_t* cproc = get_current_proc();

    uint32_t EC = (esr >> 26)&0x3F;
    uint32_t DFSC = esr & 0x3F;
    uint32_t IL = (esr >> 25) & 0x1;
    uint32_t ISS = esr & 0x1FFFFFF;

    if(EC == 0x24 || EC == 0x25){
        data_abort_handler(ctx, far, DFSC);
        return;
    }

    if(EC == 0x20 || EC == 0x21) {
        prefetch_abort_handler(ctx, DFSC);
        return;
    }

    /*
     * EC == 0 (unknown reason): the recurring D0 external-abort family
     * arrives through the SYNC vector with EC=0 (async/imprecise
     * external abort; FAR in the low-RAM 0x2xxxx-0x34xxx band whenever
     * the V3D runs).  Old behaviour dumped and killed the current
     * process, taking g2dd and bystanders down although the GPU jobs
     * complete.  Swallow it like serror_swallow: log (rate limited),
     * set PSTATE.A in the saved SPSR so it stays masked, and return to
     * the interrupted context.
     */
    if(EC == 0x00) {
        static uint32_t ec0_cnt = 0;
        uint32_t n = ++ec0_cnt;

        if(n <= 8 || (n & 0xFF) == 0)
            printf("kernel: ec0 #%u far=%08x%08x pc=%08x%08x spsr=%08x%08x pid=%d\n",
                   n,
                   SPLIT(far), SPLIT(ctx->pc), SPLIT(ctx->spsr_el1),
                   cproc ? (int)cproc->info.pid : -1);
        ctx->spsr_el1 |= 0x100u;    /* PSTATE.A: mask async aborts */
        return;
    }

    uint32_t abort_core = abort_guard_enter("sync");
    printf("\n--------------------core dump infomation------------------\n");
    /* Harden the dump: print the essential line FIRST, before the full
     * register dump - a dump_ctx that hangs on a smashed context/SP
     * must not swallow the identity of the fault. */
    if(cproc){
        printf("PID:%d CMD:%s\n", cproc->info.pid,  cproc->info.cmd);
    }
    printf("EC :%08x (%s) IL:%08x ISS:%08x FSC:%08x (%s)\n",
           EC, esr_ec_name(EC), IL, ISS, DFSC, abort_fsc_name(DFSC));
    printf("pc : %08x%08x\tlr : %08x%08x\tsp : %08x%08x\tspsr:%08x%08x\n",
           SPLIT(ctx->pc), SPLIT(ctx->lr), SPLIT(ctx->sp),
           SPLIT(ctx->spsr_el1));
    printf("ESR:%08x%08x FAR:%08x%08x\n", SPLIT(esr), SPLIT(far));
    dump_ctx(ctx);
    if(cproc != 0) {
        dump_user_fault_words(cproc, ctx);
    }
    printf("\n----------------------------------------------------------\n");
    if(abort_from_kernel(ctx)) {
        printf("kernel: sync exception from KERNEL mode (EC=%x) - kernel bug, halting\n", EC);
        while(1);
    }
    if(cproc != 0) {
        proc_exit(ctx, proc_get_proc(cproc), -1);
        abort_guard_leave(abort_core);
        return;
    }
    while(1);
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
    /*
     * AArch64 ESR_EL1.ISS[5:0] uses FSC/DFSC values:
     *   0x4..0x7  translation fault
     *   0x8..0xB  access flag fault
     *   0xC..0xF  permission fault
     * Heap COW and lazily reserved heap/stack pages can legally fault in any of
     * these groups, so treat them all as recoverable user page faults.
     */
    uint32_t fsc = status & 0x3f;
    return (uint8_t)(fsc >= 0x4 && fsc <= 0xF);
}

int abort_from_kernel(context_t* ctx) {
    if(ctx == NULL)
        return 1; /* unknown context: treat as kernel, never kill a proc on a guess */
    /* SPSR_EL1 M[3:0]: 0b0000 = EL0t (user); 0b0100/0b0101 = EL1t/h (kernel) */
    return (ctx->spsr_el1 & 0xF) != 0;
}

void arch_fill_core_dump_regs(kev_core_dump_t* dump, context_t* ctx) {
    dump->regs.aarch64.pc = ctx->pc;
    dump->regs.aarch64.spsr_el1 = ctx->spsr_el1;
    dump->regs.aarch64.sp = ctx->sp;
    dump->regs.aarch64.lr = ctx->lr;
    for(int i=0; i<30; i++)
        dump->regs.aarch64.gpr[i] = ctx->gpr[i];
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
    dump_user_fault_words(proc, ctx);
}

void arch_dump_prefetch_extra(context_t* ctx) {
    (void)ctx;
}

/*
 * <dev/timer.h>: the raw counter behind timer_read_sys_usec(), published to
 * userspace so libc can interpolate the vsyscall clock between scheduler
 * ticks. Every aarch64 target in this tree times off the ARM generic virtual
 * counter, and CNTVCT_EL0 is the same register the BSPs read, so the kernel's
 * usec and libc's interpolated nsec can never disagree about the source.
 *
 * Weak: a board whose CNTFRQ_EL0 does not match its actual counter rate can
 * override this in its BSP.
 */
__attribute__((weak)) uint32_t timer_fine_cnt(uint64_t* cnt) {
    uint64_t frq;
    __asm__ volatile("mrs %0, CNTFRQ_EL0" : "=r" (frq) : : "memory");
    if(frq == 0)
        return 0;
    if(cnt != NULL)
        __asm__ volatile("mrs %0, CNTVCT_EL0" : "=r" (*cnt) : : "memory");
    return (uint32_t)frq;
}

/*
 * <kernel/irq.h>: CNTKCTL_EL1 resets UNKNOWN, so without this a userland
 * `mrs CNTVCT_EL0` takes an EL0 trap instead of reading the counter. Per-core
 * register - the common kernel calls this on the boot core and on every AP.
 *
 * This MUST be a read-modify-write of bits [1:0] (EL0PCTEN|EL0VCTEN) and never
 * a blind store. CNTKCTL_EL1 also carries the event-stream controls (EVNTEN and
 * friends), and the kernel's global SMP spinlock in arch/v8/system.S parks on
 * `wfe` and is kicked by `sev`. Clobbering the register to 0x3 turns the event
 * stream off, which removes the periodic wakeup that makes a lost `sev`
 * self-healing - the lock then deadlocks and everything downstream of it (init's
 * exec_from_sd, vfsd's ipc_serv registration) fails. OR-ing the two enable bits
 * leaves the reset/firmware configuration of every other field untouched.
 *
 * CNTFRQ_EL0 reads are not gated by this register at all.
 */
void arch_enable_user_cnt(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, CNTKCTL_EL1" : "=r" (val) : : "memory");
    val |= 0x3; /* EL0PCTEN | EL0VCTEN */
    __asm__ volatile("msr CNTKCTL_EL1, %0":: "r"(val): "memory");
    /* context-synchronise: an EL0 counter read must not predate the enable */
    __asm__ volatile("isb" ::: "memory");
}
