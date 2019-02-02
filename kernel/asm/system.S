#include <mm/mmu.h>

.global __readControlRegister
__readControlRegister:
	mrc p15, 0, r0, cr1, cr0, 0
	mov pc, lr /*return*/

.global __setControlRegister
__setControlRegister:
	mcr p15, 0, r0, cr1, cr0, 0
	mov pc, lr

.global __setDomainAccessControl
__setDomainAccessControl:
	mcr p15, 0, r0, cr3, cr0, 0
	mov pc, lr

.global __setTranslationTableBase
__setTranslationTableBase:
	mcr p15, 0, r0, cr2, cr0, 0
	mov pc, lr

.global __jump2HighMem
__jump2HighMem:
	add lr, #KERNEL_BASE
	mov pc, lr

.global __enableInterrupts
__enableInterrupts:
	mrs r1, cpsr
	bic r1, r1, #0x80
	msr cpsr_c, r1

	mov pc, lr

.global __useHighInterrupts
__useHighInterrupts:
	push {lr}
	bl __readControlRegister
	orr r0, #0x2000
	bl __setControlRegister
	pop {lr}

	mov pc, lr

.global __switchToContext
__switchToContext:
	LDMIA R0!, {R12, R14}
	MSR SPSR_fsxc, R12
	LDMIA R0, {R0-R14}^
	NOP
	MOVS PC, R14

