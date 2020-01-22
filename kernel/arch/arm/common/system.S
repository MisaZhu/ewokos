#define KERNEL_BASE 0x80000000

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
	mcr p15, 0, r0, c2, c0, 0 //set ttbase
	mcr p15, 0, r0, c2, c0, 1 //set ttbase
	mov r0, #0
	mcr p15, 0, r0, c8, c7, 0 //flush tlb
  mcr p15, 0, r0, c7, c10, 4 // DSB ??
	mov pc, lr

.global __enable_paging
__enable_paging:
	push {lr} /*lr(R14) return PC stored*/

	mov r0, #0x1
	//bic r0, #0xc
	bl __set_domain_access_control /*system.S*/

	ldr r0, =_startup_page_dir /*kernel/src/mm/startup.c*/
	sub r0, #KERNEL_BASE /*get physical address from virtual address*/
	bl __set_translation_table_base /*system.S*/

	bl __read_control_register /*system.S*/
	orr r0, #0x1 
	//orr r0, #0x4 
	//orr r0, #0x1000 
	bl __set_control_register /*system.S*/

	pop {lr}
	mov pc, lr /*return*/

.global __jump2_high_mem
__jump2_high_mem:
	add lr, #KERNEL_BASE
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

.global __use_high_interrupts
__use_high_interrupts:
	push {lr}
	bl __read_control_register
	orr r0, #0x2000
	bl __set_control_register
	pop {lr}

	mov pc, lr

.global __mem_barrier
__mem_barrier:
	mov r0, #0 @ <Rd> should be zero (SBZ) for this
	mcr p15, 0, r0, c7, c10, 5 @ data memory barrier
	mov pc, lr

.global __dummy
__dummy:
	bx lr
