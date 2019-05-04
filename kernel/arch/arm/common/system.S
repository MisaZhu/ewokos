#include <mm/mmu.h>

.global __read_control_register
__read_control_register:
	mrc p15, 0, r0, cr1, cr0, 0 /*operate-0 from co-processor-p15 to R0, cr1 cr0 are registers on co-proc*/
	mov pc, lr /*return*/

.global __set_control_register
__set_control_register:
	mcr p15, 0, r0, cr1, cr0, 0 /*operate-0 from R0 to co-processor-p15*/
	mov pc, lr /*return*/

.global __set_domain_access_control
__set_domain_access_control:
	mcr p15, 0, r0, cr3, cr0, 0
	mov pc, lr

.global __set_translation_table_base
__set_translation_table_base:
	mcr p15, 0, r0, cr2, cr0, 0
	mov pc, lr

.global __jump2_high_mem
__jump2_high_mem:
	add lr, #KERNEL_BASE
	mov pc, lr

.global __enable_interrupts
__enable_interrupts:
	push {lr}
	mrs r1, cpsr
	bic r1, r1, #0x80
	msr cpsr_c, r1
	pop {lr}
	mov pc, lr

.global __cli 
__cli:
	push {r4}
	mrs r4, cpsr
	mov r0, r4 /*return cpsr*/
	orr r4, r4, #0x80
	msr cpsr, r4
	pop {r4}
	mov pc, lr

.global __sti
__sti:
	msr cpsr, r0
	mov pc, lr

.global __use_high_interrupts
__use_high_interrupts:
	push {lr}
	bl __read_control_register
	orr r0, #0x2000
	bl __set_control_register
	pop {lr}

	mov pc, lr

.global __switch_to_context
__switch_to_context:
	LDMIA R0!, {R12, R14}
	MSR SPSR_fsxc, R12
	LDMIA R0, {R0-R14}^
	NOP
	MOVS PC, R14

.global __mem_barrier
__mem_barrier:
	mov r0, #0 @ <Rd> should be zero (SBZ) for this
	mcr p15, 0, r0, c7, c10, 5 @ data memory barrier
	mov pc, lr
