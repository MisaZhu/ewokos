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

 /*
 * Invalidate the entire instruction cache (ARMv7-A 32-bit implementation)
 * Clobbers: r0
 */
.global __invalidate_icache_all
__invalidate_icache_all:
    // 1. Invalidate the entire instruction cache.
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 0   // ICIALLU - Invalidate all I-cache
    // 2. Make sure the operation completes.
    dsb
    isb
    // 3. Return.
    bx      lr

.global __flush_tlb
__flush_tlb:
	mov r0, #0
	/*
	 * Page-table walks are cacheable write-back inner-shareable (TTBR0
	 * IRGN/RGN/S set in __set_translation_table_base), so the walker
	 * snoops the coherent D-cache and PTE stores only need the leading
	 * dsb to be published. No whole-D-cache clean or I-cache drop belongs
	 * here; exec keeps its own code-coherency step
	 * (dcache_clean_code_range + invalidate_icache_all).
	 * TLBIALLIS broadcasts to every core of the inner-shareable domain.
	 */
	dsb
	mcr p15, 0, r0, c8, c3, 0    // invalidate all TLB, inner-shareable
	dsb
	isb
	mov pc, lr

.global __set_translation_table_base
__set_translation_table_base:
	/*
	 * TTBR0 attribute bits: IRGN=write-back write-allocate (bit0),
	 * S=shareable (bit1), RGN=write-back write-allocate (bit3). This makes
	 * page-table walks cacheable + inner-shareable so the walker snoops the
	 * coherent D-cache; SCTLR.N is left 0 by boot, and the first-level table
	 * is 16KB-aligned (base bits[13:0]=0), so ORing 0x0B is safe.
	 */
	orr r0, r0, #0x0B
	mcr p15, 0, r0, c2, c0, 0    //set ttbase user
	//mcr p15, 0, r0, c2, c0, 1    //set ttbase kernel
	dmb
	mov pc, lr

/*
 * void __set_translation_table_base_asid(uint32_t base, uint32_t asid)
 * Address-space switch without a TLB invalidate. TTBR0 gets the same cacheable
 * inner-shareable attributes as __set_translation_table_base; CONTEXTIDR[7:0]
 * carries the ASID. User-half entries are nG (ASID-tagged, see mmu_arch.c) and
 * the kernel half is global, so the previous space's entries simply stop
 * matching and no TLBI is needed. The isb publishes the new context. asid 0 is
 * reserved for the kernel/no-space fallback.
 */
.global __set_translation_table_base_asid
__set_translation_table_base_asid:
	orr r0, r0, #0x0B
	mcr p15, 0, r0, c2, c0, 0    // TTBR0 = base | IRGN/S/RGN
	mcr p15, 0, r1, c13, c0, 1   // CONTEXTIDR = ASID (bits[7:0])
	isb
	mov pc, lr

/*
 * void __flush_tlb_asid(uint32_t asid)
 * Drop every TLB entry tagged with one ASID on all cores of the inner-shareable
 * domain (TLBIASIDIS, Rt = ASID). Used when a pde slot - and thus its ASID - is
 * reassigned or its page tables are freed, so a recycled ASID never resolves
 * through the previous owner's leftovers.
 */
.global __flush_tlb_asid
__flush_tlb_asid:
	dsb
	mcr p15, 0, r0, c8, c3, 2    // TLBIASIDIS
	dsb
	isb
	mov pc, lr

/*
 * Invalidate the whole I-cache on every core of the inner-shareable domain
 * (ICIALLUIS). Used after loading code so a core other than the loader does
 * not fetch stale instructions from a reused physical page.
 */
.global __invalidate_icache_all_is
__invalidate_icache_all_is:
	mov r0, #0
	mcr p15, 0, r0, c7, c1, 0    // ICIALLUIS
	dsb
	isb
	mov pc, lr

/*
 * void __dcache_flush_poc_range(uint32_t start, uint32_t end)
 * Clean & invalidate a VA range to the point of coherency (DCCIMVAC), for
 * data the kernel wrote through a cacheable alias that a non-coherent master
 * (VC4 g2d, dma engine) or a Non-Cacheable alias will read from DRAM. Mirrors
 * aarch64 DC CIVAC. Line size from CTR.DminLine. Caller guarantees start<end.
 */
.global __dcache_flush_poc_range
__dcache_flush_poc_range:
	mrc p15, 0, r2, c0, c0, 1    // read CTR
	lsr r2, r2, #16
	and r2, r2, #0xf             // DminLine: log2(words)
	mov r3, #4
	lsl r3, r3, r2               // line size in bytes = 4 << DminLine
	sub r2, r3, #1
	bic r0, r0, r2               // align start down to a line
1:	mcr p15, 0, r0, c7, c14, 1   // DCCIMVAC
	add r0, r0, r3
	cmp r0, r1
	blo 1b
	dsb
	mov pc, lr

/*
 * void __dcache_clean_pou_range(uint32_t start, uint32_t end)
 * Clean freshly written code to the point of unification (DCCMVAU) so an
 * instruction fetch that does not snoop the L1 D-cache sees it. Mirrors
 * aarch64 DC CVAU. Caller guarantees start<end.
 */
.global __dcache_clean_pou_range
__dcache_clean_pou_range:
	mrc p15, 0, r2, c0, c0, 1    // read CTR
	lsr r2, r2, #16
	and r2, r2, #0xf             // DminLine: log2(words)
	mov r3, #4
	lsl r3, r3, r2               // line size in bytes = 4 << DminLine
	sub r2, r3, #1
	bic r0, r0, r2               // align start down to a line
1:	mcr p15, 0, r0, c7, c11, 1   // DCCMVAU
	add r0, r0, r3
	cmp r0, r1
	blo 1b
	dsb
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

.global __int_off
__int_off:
	mrs r0, cpsr
	mov r1, r0
	orr r1, r1, #0x80
	msr cpsr_c, r1
	mov pc, lr

.global __int_on
__int_on:
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
