#define KERNEL_BASE 0x80000000

.section ".init"
.global __entry
__entry:
	// Check for HYP mode
	mrs r0, cpsr_all
	and r0, r0, #0x1F
	mov r8, #0x1A
	cmp r0, r8
	beq over_hyped
	b continue

over_hyped: // Get out of HYP mode 
	ldr r1, =continue
	msr ELR_hyp, r1
	mrs r1, cpsr_all
	and r1, r1, #0x1f  //CPSR_MODE_MASK
	orr r1, r1, #0x13	 //CPSR_MODE_SUPERVISOR
	msr SPSR_hyp, r1
	eret

continue:
	// Shut off extra cores
	mrc p15, 0, r5, c0, c0, 5
	and r5, r5, #3
	cmp r5, #0
	bne halt

	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_stack        @ initialise SVC mode stack
	sub sp, #KERNEL_BASE

	bl arm_smp_disable
	bl cpu_mmu_disable
	bl cpu_dcache_disable
	bl cpu_icache_disable

	blx _boot_start

	msr cpsr, #0xD2             @ enter IRQ mode with IRQ and FIQ interrupts disabled
	ldr sp, = _irq_stack        @ initialise IRQ mode stack

	msr cpsr, #0xD7             @ enter ABT mode with IRQ and FIQ interrupts disabled
	ldr sp, = _abt_stack        @ initialise ABT mode stack

	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_stack        @ initialise SVC mode stack

	sub   sp, sp, #68             @ initialise dummy context
	mov   r0, sp                  @ set    high-level C function arg. = SP

	blx _kernel_entry_c

	ldmia sp!, { r0, lr }         @ load   USR mode PC and CPSR
	msr   spsr, r0                @ set    USR mode        CPSR
	ldmia sp, { r0-r12, sp, lr }^ @ load   USR mode registers
	add   sp, sp, #60             @ update SVC mode SP
	movs  pc, lr                  @ return from interrupt

halt:
	b halt
	
/*copy from RT-Thread (https://github.com/BernardXiong/rt-thread/)*/
.globl arm_smp_disable
arm_smp_disable:
	mrc p15, 0, r0, c1, c0, 1   // clear SMP bit in ACTLR
	bic r0, r0, #0x40
	mcr p15, 0, r0, c1, c0, 1
	bx lr

.globl cpu_mmu_disable
cpu_mmu_disable:
	mcr     p15, #0, r0, c8, c7, #0    @ invalidate tlb
	mrc     p15, #0, r0, c1, c0, #0
	bic     r0, r0, #1
	mcr     p15, #0, r0, c1, c0, #0    @ clear mmu bit
	dsb
	bx      lr

_FLD_MAX_WAY:
 .word  0x3ff
_FLD_MAX_IDX:
 .word  0x7ff

.local boot_cpu_dcache_clean_flush
boot_cpu_dcache_clean_flush:
	push    {r4-r11}
	dmb
	mrc     p15, #1, r0, c0, c0, #1  @ read clid register
	ands    r3, r0, #0x7000000       @ get level of coherency
	mov     r3, r3, lsr #23
	beq     finished
	mov     r10, #0
loop1:
	add     r2, r10, r10, lsr #1
	mov     r1, r0, lsr r2
	and     r1, r1, #7
	cmp     r1, #2
	blt     skip
	mcr     p15, #2, r10, c0, c0, #0
	isb
	mrc     p15, #1, r1, c0, c0, #0
	and     r2, r1, #7
	add     r2, r2, #4
	ldr     r4, _FLD_MAX_WAY
	ands    r4, r4, r1, lsr #3
	clz     r5, r4
	ldr     r7, _FLD_MAX_IDX
	ands    r7, r7, r1, lsr #13
loop2:
	mov     r9, r4
loop3:
	orr     r11, r10, r9, lsl r5
	orr     r11, r11, r7, lsl r2
	mcr     p15, #0, r11, c7, c14, #2
	subs    r9, r9, #1
	bge     loop3
	subs    r7, r7, #1
	bge     loop2
	skip:
	add     r10, r10, #2
	cmp     r3, r10
	bgt     loop1

finished:
	dsb
	isb
	pop     {r4-r11}
	bx      lr

.local cpu_dcache_disable
cpu_dcache_disable:
	push    {r4-r11, lr}
	bl      boot_cpu_dcache_clean_flush
	mrc     p15, #0, r0, c1, c0, #0
	bic     r0,  r0, #0x00000004
	mcr     p15, #0, r0, c1, c0, #0
	pop     {r4-r11, lr}
	bx      lr

.local cpu_icache_disable
cpu_icache_disable:
	mrc     p15, #0, r0, c1, c0, #0
	bic     r0,  r0, #0x00001000
	mcr     p15, #0, r0, c1, c0, #0
	bx      lr
