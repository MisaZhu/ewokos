.global __cpu_dcache_clean_flush
__cpu_dcache_clean_flush:
	bx lr

.global	__flush_dcache_all
__flush_dcache_all:
	/*
	 * ARM1136/ARM1176 implement whole-cache clean ops directly, so unlike
	 * ARM926 (v5) there is no test-and-clean loop here: c7,c14,0 cleans and
	 * invalidates the entire D-cache, c7,c10,4 drains the write buffer.
	 * Must run before __flush_tlb() drops the TLB or any line still dirty
	 * in the D-cache would be lost with the page-table write inside it.
	 */
	mov r0, #0
	mcr p15, 0, r0, c7, c14, 0   @ clean and invalidate whole D-cache
	mcr p15, 0, r0, c7, c10, 4   @ drain write buffer
	bx lr
 
.global	__invalidate_dcache_all
__invalidate_dcache_all:
	mov r0, #0
	mcr p15, 0, r0, c7, c6, 0    @ invalidate whole D-cache (discards dirt)
	bx lr

.global __invalidate_icache_all
__invalidate_icache_all:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0    @ invalidate whole I-cache
	mcr p15, 0, r0, c7, c5, 6    @ invalidate branch predictor
	bx lr

/*
 * Clean & invalidate a VA range to the point of coherency (DCCIMVAC), the
 * v6 counterpart of v7's __dcache_flush_poc_range. ARM1176 has a 32-byte line.
 * r0 = start (any alignment), r1 = end. Clobbers r0-r3.
 */
.global __dcache_flush_poc_range
__dcache_flush_poc_range:
	bic r0, r0, #31
1:	mcr p15, 0, r0, c7, c14, 1   @ clean and invalidate D-cache line by MVA
	add r0, r0, #32
	cmp r0, r1
	blo 1b
	mov r3, #0
	mcr p15, 0, r3, c7, c10, 4   @ drain write buffer
	bx lr

/*
 * Clean a VA range to the point of unification (DCCMUAC) and drop any I-cache
 * line for it, so code written through a cacheable kernel alias is fetched
 * correctly afterwards. r0 = start, r1 = end. Clobbers r0-r3.
 */
.global __dcache_clean_pou_range
__dcache_clean_pou_range:
	bic r2, r0, #31              @ aligned start, kept for the I-cache pass
	mov r0, r2
1:	mcr p15, 0, r0, c7, c10, 1   @ clean D-cache line by MVA
	add r0, r0, #32
	cmp r0, r1
	blo 1b
	mov r3, #0
	mcr p15, 0, r3, c7, c10, 4   @ drain write buffer
2:	mcr p15, 0, r2, c7, c5, 1    @ invalidate I-cache line by MVA
	add r2, r2, #32
	cmp r2, r1
	blo 2b
	mcr p15, 0, r3, c7, c10, 4   @ dsb
	mcr p15, 0, r3, c7, c5, 4    @ isb
	bx lr

.global __flush_tlb
__flush_tlb:
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 4 // dsb
	mcr p15, 0, r0, c7, c5, 4  // isb
	mcr p15, 0, r0, c7, c5, 0    // flush icache all
	mcr p15, 0, r0, c7, c5, 6    // flush branch prediction
	mcr p15, 0, r0, c8, c7, 0    //unified tlb invalidation
	mcr p15, 0, r0, c8, c6, 0    //data tlb invalidation
	mcr p15, 0, r0, c8, c5, 0    //instruction tlb invalidation
	mcr p15, 0, r0, c7, c10, 4 // dsb
	mcr p15, 0, r0, c7, c5, 4  // isb
	mov pc, lr

.global __set_translation_table_base
__set_translation_table_base:
	mcr p15, 0, r0, c2, c0, 0    //set ttbase user
	mcr p15, 0, r0, c2, c0, 1    //set ttbase kernel
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
