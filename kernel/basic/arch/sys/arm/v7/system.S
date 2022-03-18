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

_FLD_MAX_WAY:
 .word  0x3ff
_FLD_MAX_IDX:
 .word  0x7ff

/*copy from RT-Thread 
(https://github.com/BernardXiong/rt-thread/bsp/raspberry-pi/raspi2/cpu)
Exchanged with Bernard by one cup of coffee*/
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
	dmb
	dsb
	isb                         // instruction sync
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

.global __read_current_freq
__read_current_freq:
  mcr p15, 0, r0, c14, c0, 0
  mov pc, lr

.global __write_cntv_tval
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
