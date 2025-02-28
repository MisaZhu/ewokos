/*
 * Wrap c macros in asm macros to delay expansion until after the
 * SMCCC asm macro is expanded.
 */
    .macro SMCCC_SMC
    smc #0
    .endm

    .macro SMCCC instr
    mov r12, sp
    push    {r4-r7}
    ldm r12, {r4-r7}
    \instr
    pop {r4-r7}
    ldr r12, [sp, #(4 * 4)]
    stm r12, {r0-r3}
    bx  lr
    .endm

/*
 * void smccc_smc(unsigned long a0, unsigned long a1, unsigned long a2,
 *        unsigned long a3, unsigned long a4, unsigned long a5,
 *        unsigned long a6, unsigned long a7, struct arm_smccc_res *res)
 */
.global arm_smccc_smc
.type   arm_smccc_smc, function; 
arm_smccc_smc:
        SMCCC SMCCC_SMC
