.global __cpu_dcache_clean_flush
__cpu_dcache_clean_flush:
	bx lr

.global	__flush_dcache_all
__flush_dcache_all:
	mov r0, #0
	MRC p15, 0, r0, c7, c10, 3
	MCR p15, 0, r0, c7, c6, 0
	bx lr
/*
.global	__invalidate_dcache_all
__invalidate_dcache_all:
	mov r0, #0
	MCR p15, 0, r0, c7, c6, 0
	bx lr
*/

.global __flush_tlb
__flush_tlb:
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 4 // dsb
#	mcr p15, 0, r0, c7, c5, 4  // isb
	mcr p15, 0, r0, c7, c5, 0  // flush icache all
#	mcr p15, 0, r0, c7, c5, 6  // flush branch prediction
	mcr p15, 0, r0, c7, c7, 0  // invalidate both data and instruction
#	mcr p15, 0, r0, c8, c7, 0  //unified tlb invalidation
	mcr p15, 0, r0, c8, c6, 0  //data tlb invalidation
#	mcr p15, 0, r0, c8, c5, 0  //instruction tlb invalidation
	mcr p15, 0, r0, c7, c10, 4 // dsb
#	mcr p15, 0, r0, c7, c5, 4  // isb
	mov pc, lr

.global __set_translation_table_base
__set_translation_table_base:
	mcr p15, 0, r0, c2, c0, 0    //set ttbase user
#	mcr p15, 0, r0, c2, c0, 1    //set ttbase kernel
	mov pc, lr

.global __set_vector_table
__set_vector_table:
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
	mov r0, #0                 @ <Rd> should be zero (SBZ) for this
	mcr p15, 0, r0, c7, c10, 5 @ data memory barrier
	mov pc, lr
