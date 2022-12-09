#include <kernel/irq.h>
#include <mm/mmu.h>
#include <csr.h>
#include <kernel/svc.h>
#include <kernel/proc.h>
#include <arch_context.h>
#include <dev/timer.h>

inline void irq_enable_cpsr(context_t* ctx) {

}

inline void irq_disable_cpsr(context_t* ctx) {

}

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
