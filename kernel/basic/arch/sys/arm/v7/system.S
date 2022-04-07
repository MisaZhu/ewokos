#define KERNEL_BASE 0x80000000

.global __core_id
__core_id:
	mrc p15, 0, r0, c0, c0, 5
	and r0, r0, #3
	mov pc, lr

.global __cpu_cores
__cpu_cores:
	mov r0, #4
	mov pc, lr

/*
 *************************************************************************
 *
 * invalidate_dcache - invalidate the entire d-cache by set/way
 *
 * Note: for Cortex-A53, there is no cp instruction for invalidating
 * the whole D-cache. Need to invalidate each line.
 *
 *************************************************************************
 */
.global __cpu_dcache_clean_flush
__cpu_dcache_clean_flush:
	push	{r0 - r12}
	mrc	p15, 1, r0, c0, c0, 1		/* read CLIDR */
	ands	r3, r0, #0x7000000
	mov	r3, r3, lsr #23			/* cache level value (naturally aligned) */
	beq	finished
	mov	r10, #0				/* start with level 0 */
loop1:
	add	r2, r10, r10, lsr #1		/* work out 3xcachelevel */
	mov	r1, r0, lsr r2			/* bottom 3 bits are the Cache type for this level */
	and	r1, r1, #7			/* get those 3 bits alone */
	cmp	r1, #2
	blt	skip				/* no cache or only instruction cache at this level */
	mcr	p15, 2, r10, c0, c0, 0		/* write the Cache Size selection register */
	isb					/* isb to sync the change to the CacheSizeID reg */
	mrc	p15, 1, r1, c0, c0, 0		/* reads current Cache Size ID register */
	and	r2, r1, #7			/* extract the line length field */
	add	r2, r2, #4			/* add 4 for the line length offset (log2 16 bytes) */
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3		/* r4 is the max number on the way size (right aligned) */
	clz	r5, r4				/* r5 is the bit position of the way size increment */
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13		/* r7 is the max number of the index size (right aligned) */
loop2:
	mov	r9, r4				/* r9 working copy of the max way size (right aligned) */
loop3:
	orr	r11, r10, r9, lsl r5		/* factor in the way number and cache number into r11 */
	orr	r11, r11, r7, lsl r2		/* factor in the index number */
	mcr	p15, 0, r11, c7, c6, 2		/* invalidate by set/way */
	subs	r9, r9, #1			/* decrement the way number */
	bge	loop3
	subs	r7, r7, #1			/* decrement the index */
	bge	loop2
skip:
	add	r10, r10, #2			/* increment the cache number */
	cmp	r3, r10
	bgt	loop1

finished:
	mov	r10, #0				/* swith back to cache level 0 */
	mcr	p15, 2, r10, c0, c0, 0		/* select current cache level in cssr */
	dsb
	isb

	pop {r0-r12}
	bx	lr

.global __flush_tlb
__flush_tlb:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0    // flush icache all
	//mcr p15, 0, r0, c7, c7, 0  //invalidate cache
	mcr p15, 0, r0, c8, c7, 0    //I-TLB and D-TLB invalidation
	dsb
	isb                          // instruction sync
	mov pc, lr

.global __set_translation_table_base
__set_translation_table_base:
	mcr p15, 0, r0, c2, c0, 0    //set ttbase user
	mcr p15, 0, r0, c2, c0, 1    //set ttbase kernel
	dsb
	isb
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

.global __write_cntv_tval         //timer_tval = current_freq / timers_per_second
__write_cntv_tval:
  mcr p15, 0, r0, c14, c3, 0
  mov pc, lr

.global __enable_cntv
__enable_cntv:
  mov r0, #1;
  mcr p15, 0, r0, c14, c3, 1
  mov pc, lr

.global __disable_cntv
__disable_cntv:
  mov r0, #0;
  mcr p15, 0, r0, c14, c3, 1
  mov pc, lr

.global __smp_lock
__smp_lock:
	ldrex r1, [r0]
	cmp r1, #0x0
	WFENE
	bne __smp_lock
	mov r1, #1
	strex r2, r1, [r0]
	cmp r2, #0x0
	bne __smp_lock
	DMB
	bx lr

.global __smp_unlock
__smp_unlock:
	mov r1, #0x0
	DMB
	str r1, [r0]
	DSB
	SEV
	bx lr

.global __enable_scu
__enable_scu:
	mrc p15, 4, r0, c15, c0, 0
	ldr r1, [r0, #0x0]
	orr r1, r1, #0x01
	str r1, [r0, #0x0]
	bx lr
