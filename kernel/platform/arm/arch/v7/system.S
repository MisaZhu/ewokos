.global __core_id
__core_id:
	mrc p15, 0, r0, c0, c0, 5
	and r0, r0, #0xff
	mov pc, lr

#define ENTRY(symbol)                    \
        .globl  symbol;                  \
        .type   symbol, function;        \
symbol:
#define ENDPROC(x)

/*
 *    v7_flush_dcache_all()
 *
 *    Flush the whole D-cache.
 *
 *    Corrupted registers: r0-r5, r7, r9-r11
 *
 *    Note: copied from arch/arm/mm/cache-v7.S of Linux 4.4
 */
ENTRY(__v7_flush_dcache_all)
    dmb                               @ ensure ordering with previous memory accesses
    mrc    p15, 1, r0, c0, c0, 1      @ read clidr
    mov    r3, r0, lsr #23            @ move LoC into position
    ands    r3, r3, #7 << 1           @ extract LoC*2 from clidr
    beq    finished                   @ if loc is 0, then no need to clean
start_flush_levels:
    mov    r10, #0                    @ start clean at cache level 0
flush_levels:
    add    r2, r10, r10, lsr #1       @ work out 3x current cache level
    mov    r1, r0, lsr r2             @ extract cache type bits from clidr
    and    r1, r1, #7                 @ mask of the bits for current cache only
    cmp    r1, #2                     @ see what cache we have at this level
    blt    skip                       @ skip if no cache, or just i-cache
    mcr    p15, 2, r10, c0, c0, 0     @ select current cache level in cssr
    isb                               @ isb to sych the new cssr&csidr
    mrc    p15, 1, r1, c0, c0, 0      @ read the new csidr
    and    r2, r1, #7                 @ extract the length of the cache lines
    add    r2, r2, #4                 @ add 4 (line length offset)
    movw    r4, #0x3ff
    ands    r4, r4, r1, lsr #3        @ find maximum number on the way size
    clz    r5, r4                     @ find bit position of way size increment
    movw    r7, #0x7fff
    ands    r7, r7, r1, lsr #13       @ extract max number of the index size
loop1:
    mov    r9, r7                     @ create working copy of max index
loop2:
    orr    r11, r10, r4, lsl r5       @ factor way and cache number into r11


    orr    r11, r11, r9, lsl r2       @ factor index number into r11


    mcr    p15, 0, r11, c7, c14, 2    @ clean & invalidate by set/way
    subs    r9, r9, #1                @ decrement the index
    bge    loop2
    subs    r4, r4, #1                @ decrement the way
    bge    loop1
skip:
    add    r10, r10, #2               @ increment cache number
    cmp    r3, r10
    bgt    flush_levels
finished:
    mov    r10, #0                    @ swith back to cache level 0
    mcr    p15, 2, r10, c0, c0, 0     @ select current cache level in cssr
    dsb    st
    isb
    bx    lr
ENDPROC(__v7_flush_dcache_all)

ENTRY(__flush_dcache_all)
    stmfd    sp!, {r4-r5, r7, r9-r11, lr}

    bl    __v7_flush_dcache_all
    ldmfd    sp!, {r4-r5, r7, r9-r11, lr}

    bx    lr
ENDPROC(__flush_dcache_all)

/*
 *    v7_invalidate_dcache_all()
 *
 *    Invalidate the whole D-cache.
 *
 *    Corrupted registers: r0-r5, r7, r9-r11
 *
 *    Note: copied from __v7_flush_dcache_all above with
 *    mcr     p15, 0, r11, c7, c14, 2
 *    Replaced with:
 *    mcr     p15, 0, r11, c7, c6, 2
 */

/*
ENTRY(__v7_invalidate_dcache_all)
    dmb                             @ ensure ordering with previous memory accesses
    mrc    p15, 1, r0, c0, c0, 1    @ read clidr
    mov    r3, r0, lsr #23          @ move LoC into position
    ands    r3, r3, #7 << 1         @ extract LoC*2 from clidr
    beq    inval_finished           @ if loc is 0, then no need to clean
    mov    r10, #0                  @ start clean at cache level 0
inval_levels:
    add    r2, r10, r10, lsr #1     @ work out 3x current cache level
    mov    r1, r0, lsr r2           @ extract cache type bits from clidr
    and    r1, r1, #7               @ mask of the bits for current cache only
    cmp    r1, #2                   @ see what cache we have at this level
    blt    inval_skip               @ skip if no cache, or just i-cache
    mcr    p15, 2, r10, c0, c0, 0   @ select current cache level in cssr
    isb                             @ isb to sych the new cssr&csidr
    mrc    p15, 1, r1, c0, c0, 0    @ read the new csidr
    and    r2, r1, #7               @ extract the length of the cache lines
    add    r2, r2, #4               @ add 4 (line length offset)
    movw    r4, #0x3ff
    ands    r4, r4, r1, lsr #3      @ find maximum number on the way size
    clz    r5, r4                   @ find bit position of way size increment
    movw    r7, #0x7fff
    ands    r7, r7, r1, lsr #13     @ extract max number of the index size
inval_loop1:
    mov    r9, r7                   @ create working copy of max index
inval_loop2:
    orr    r11, r10, r4, lsl r5     @ factor way and cache number into r11


    orr    r11, r11, r9, lsl r2     @ factor index number into r11


    mcr    p15, 0, r11, c7, c6, 2   @ invalidate by set/way
    subs    r9, r9, #1              @ decrement the index
    bge    inval_loop2
    subs    r4, r4, #1              @ decrement the way
    bge    inval_loop1
inval_skip:
    add    r10, r10, #2             @ increment cache number
    cmp    r3, r10
    bgt    inval_levels
inval_finished:
    mov    r10, #0                  @ swith back to cache level 0
    mcr    p15, 2, r10, c0, c0, 0   @ select current cache level in cssr
    dsb    st
    isb
    bx    lr
ENDPROC(__v7_invalidate_dcache_all)

ENTRY(__invalidate_dcache_all)
    stmfd    sp!, {r4-r5, r7, r9-r11, lr}
    bl    __v7_invalidate_dcache_all
    ldmfd    sp!, {r4-r5, r7, r9-r11, lr}
    bx    lr
ENDPROC(__invalidate_dcache_all)

 */
 /*
 * 无效化整个指令缓存 (ARMv7-A 32位实现)
 * 破坏寄存器：r0
 */
.global __invalidate_icache_all
__invalidate_icache_all:
    // 1. 无效化整个指令缓存
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 0   // ICIALLU - Invalidate all I-cache
    // 2. 确保操作完成
    dsb
    isb
    // 3. 返回
    bx      lr

.global __flush_tlb
__flush_tlb:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0    // flush icache all
	mcr p15, 0, r0, c7, c5, 6    // flush branch prediction
	mcr p15, 0, r0, c8, c7, 0    //unified tlb invalidation
	mcr p15, 0, r0, c8, c6, 0    //data tlb invalidation
	mcr p15, 0, r0, c8, c5, 0    //instruction tlb invalidation
	isb
	dsb
	mov pc, lr

.global __set_translation_table_base
__set_translation_table_base:
	mcr p15, 0, r0, c2, c0, 0    //set ttbase user
	//mcr p15, 0, r0, c2, c0, 1    //set ttbase kernel
	dmb
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
