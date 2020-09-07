#define KERNEL_BASE 0x80000000

_FLD_MAX_WAY:
 .word  0x3ff
_FLD_MAX_IDX:
 .word  0x7ff

.global __cpu_dcache_clean_flush
__cpu_dcache_clean_flush:
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

.global __flush_tlb
__flush_tlb:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0   // flush icache all
	mcr p15, 0, r0, c8, c7, 0   //I-TLB and D-TLB invalidation
	dsb
	mov pc, lr

.global __set_translation_table_base
__set_translation_table_base:
	mcr p15, 0, r0, c2, c0, 0  //set ttbase user
	mcr p15, 0, r0, c2, c0, 1  //set ttbase kernel
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
