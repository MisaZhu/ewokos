#include <kernel/irq.h>
#include <mm/mmu.h>
#include <csr.h>
#include <kernel/svc.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <arch_context.h>
#include <dev/timer.h>
#include <stdint.h>

void dump_ctx(context_t *ctx)
{
    printf("\nSP:  " "%016x" " GP:  " "%016x" " TP:  " "%016x" "\n",
           ctx->sp, ctx->gp, ctx->tp);
    printf("T0:  " "%016x" " T1:  " "%016x" " T2:  " "%016x" "\n",
           ctx->t0, ctx->t1, ctx->t2);
    printf("S0:  " "%016x" " S1:  " "%016x" " A0:  " "%016x" "\n",
           ctx->s0, ctx->s1, ctx->gpr[0]);
    printf("A1:  " "%016x" " A2:  " "%016x" " A3:  " "%016x" "\n",
           ctx->gpr[1], ctx->gpr[2], ctx->gpr[3]);
    printf("A4:  " "%016x" " A5:  " "%016x" " A6:  " "%016x" "\n",
           ctx->gpr[4], ctx->gpr[5], ctx->gpr[6]);
    printf("A7:  " "%016x" " S2:  " "%016x" " S3:  " "%016x" "\n",
           ctx->gpr[7], ctx->gpr[8], ctx->gpr[9]);
    printf("S4:  " "%016x" " S5:  " "%016x" " S6:  " "%016x" "\n",
           ctx->gpr[10], ctx->gpr[11], ctx->gpr[12]);
    printf("S7:  " "%016x" " S8:  " "%016x" " S9:  " "%016x" "\n",
           ctx->gpr[13], ctx->gpr[14], ctx->gpr[15]);
    printf("S10: " "%016x" " S11: " "%016x" " T3:  " "%016x" "\n",
           ctx->gpr[16], ctx->gpr[17], ctx->gpr[18]);
    printf("T4:  " "%016x" " T5:  " "%016x" " T6:  " "%016x" "\n",
           ctx->gpr[19], ctx->gpr[20], ctx->gpr[21]);
}

static int instr_len(uint16_t i)
{
    if ((i & 0x03) != 0x03)
        return 1;
    /* Instructions with more than 32 bits are not yet specified */
    return 2;
}

static void show_code(uint64_t epc)
{
    uint16_t *pos = (uint16_t *)(epc & ~1UL);
    int i, len = instr_len(*pos);

    printf("\nCode: ");
    for (i = -8; i; ++i)
        printf("%04x ", pos[i]);
    printf("(");
    for (i = 0; i < len; ++i)
        printf("%04x%s", pos[i], i + 1 == len ? ")\n" : " ");
}



void panic(uint32_t code, uint64_t epc, uint64_t tval,  context_t *ctx)
{

    static const char * const exception_code[] = {
        "Instruction address misaligned",
        "Instruction access fault",
        "Illegal instruction",
        "Breakpoint",
        "Load address misaligned",
        "Load access fault",
        "Store/AMO address misaligned",
        "Store/AMO access fault",
        "Environment call from U-mode",
        "Environment call from S-mode",
        "Reserved",
        "Environment call from M-mode",
        "Instruction page fault",
        "Load page fault",
        "Reserved",
        "Store/AMO page fault",
    };

    proc_t *proc = get_current_proc();

    if (code < 16L)
       printf("Unhandled exception: %s\n", exception_code[code]);
    else
       printf("Unhandled exception code: %d\n", code);

    printf("PROC: %s EPC: " "%016x" " RA: " "%016x" " TVAL: " "%016x" "\n",
           proc?proc->info.cmd:"", epc, ctx->ra, tval);

    dump_ctx(ctx);
    show_code(epc);
    sbi_srst_reset(0);
    while(1);
}


uint64_t handle_trap(uint32_t cause, uint64_t epc, uint64_t tval,  context_t *ctx)
{
    switch(cause){
       case 5:
              timer_clear_interrupt(0);
              irq_handler(ctx);
           break;
       case 8:
              ctx->pc += 4;
              svc_handler(ctx->gpr[0], ctx->gpr[1], ctx->gpr[2], ctx->gpr[3], ctx); 
              break;
        case 15:
              data_abort_handler(ctx, tval, 0x6);
              break;
       default:
              panic(cause, epc, tval, ctx);
              break;
    }
    return epc;
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
    /* sstatus.SPP (bit 8): 1 = trapped from S-mode (kernel), 0 = U-mode */
    return (ctx->sstatus & (1UL << 8)) != 0;
}

void arch_fill_core_dump_regs(kev_core_dump_t* dump, context_t* ctx) {
    dump->regs.riscv.pc = ctx->pc;
    dump->regs.riscv.ra = ctx->ra;
    dump->regs.riscv.sp = ctx->sp;
    dump->regs.riscv.gp = ctx->gp;
    dump->regs.riscv.tp = ctx->tp;
    dump->regs.riscv.t0 = ctx->t0;
    dump->regs.riscv.t1 = ctx->t1;
    dump->regs.riscv.t2 = ctx->t2;
    dump->regs.riscv.s0 = ctx->s0;
    dump->regs.riscv.s1 = ctx->s1;
    for(int i=0; i<8; i++)
        dump->regs.riscv.gpr[i] = ctx->gpr[i];
    dump->regs.riscv.s2 = ctx->s2;
    dump->regs.riscv.s3 = ctx->s3;
    dump->regs.riscv.s4 = ctx->s4;
    dump->regs.riscv.s5 = ctx->s5;
    dump->regs.riscv.s6 = ctx->s6;
    dump->regs.riscv.s7 = ctx->s7;
    dump->regs.riscv.s8 = ctx->s8;
    dump->regs.riscv.s9 = ctx->s9;
    dump->regs.riscv.s10 = ctx->s10;
    dump->regs.riscv.s11 = ctx->s11;
    dump->regs.riscv.t3 = ctx->t3;
    dump->regs.riscv.t4 = ctx->t4;
    dump->regs.riscv.t5 = ctx->t5;
    dump->regs.riscv.t6 = ctx->t6;
    dump->regs.riscv.sstatus = ctx->sstatus;
    dump->regs.riscv.sbadaddr = ctx->sbadaddr;
    dump->regs.riscv.scause = ctx->scause;
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
    /* riscv builds never enabled the user-word dump */
    (void)proc;
    (void)ctx;
}

void arch_dump_prefetch_extra(context_t* ctx) {
    (void)ctx;
}

/*
 * <dev/timer.h>: the `time` CSR, published to userspace so libc can
 * interpolate the vsyscall clock between scheduler ticks.
 *
 * The frequency must match the one machines/virt.riscv/kernel/bsp/timer.c
 * divides by in timer_read_sys_usec() (`csr_read(CSR_TIME)/10`), otherwise
 * libc's interpolated nsec drifts away from the kernel's own usec. QEMU virt
 * runs the timebase at exactly 10MHz; a board with a different timebase should
 * override this weak symbol in its BSP rather than change the constant.
 */
#define RISCV_TIMEBASE_HZ 10000000U

__attribute__((weak)) uint32_t timer_fine_cnt(uint64_t* cnt) {
    if(cnt != NULL)
        *cnt = (uint64_t)csr_read(CSR_TIME);
    return RISCV_TIMEBASE_HZ;
}

/*
 * <kernel/irq.h>: a U-mode `rdtime` traps unless scounteren.TM is set. OpenSBI
 * already sets mcounteren.TM (it enables cycle/time/instret by default), so
 * this S-mode bit is the only missing link. Per-core CSR, but scounteren is
 * written by each hart on entry - the common kernel calls this per core.
 */
void arch_enable_user_cnt(void) {
    csr_set(CSR_SCOUNTEREN, 0x2);   /* TM: bit 1 */
}
