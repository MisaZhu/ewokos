#define KERNEL_BASE 0x80000000

.global __set_translation_table_base
__set_translation_table_base:
	mcr p15, 0, r0, c2, c0, 0 //set ttbase user
	mcr p15, 0, r0, c2, c0, 1 //set ttbase kernel
	mov r0, #0
	mcr p15, 0, r0, c8, c7, 0 //flush tlb
  //mcr p15, 0, r0, c7, c10, 4 // DSB ??
	mov pc, lr

.global __irq_enable
__irq_enable:
	mrs r0, cpsr
	bic r0, r0, #0x80
	msr cpsr_c, r0
	mov pc, lr

.global __irq_disable
__irq_disable:
	mrs r0, cpsr
	orr r0, r0, #0x80
	msr cpsr_c, r0
	mov pc, lr

.global __int_off // SR = int_off()
__int_off: 
	mrs r0, cpsr
	mov r1, r0
	orr r1, r1, #0x80
	msr cpsr, r1
  mov pc, lr

.global __int_on
__int_on:
	msr cpsr, r0
  mov pc, lr

.global __mem_barrier
__mem_barrier:
	mov r0, #0 @ <Rd> should be zero (SBZ) for this
	mcr p15, 0, r0, c7, c10, 5 @ data memory barrier
	mov pc, lr

.global __dummy
__dummy:
	bx lr
