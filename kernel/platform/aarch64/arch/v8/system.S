.global __core_id
__core_id:
    /* core id: Aff0 on A53/A72 (virt/pi4), Aff1 on DynamIQ A76 (pi5);
       the unused level reads as 0, so merging both yields the core number */
    mrs x0, mpidr_el1
    ubfx x1, x0, #8, #8
    and x0, x0, #0xFF
    orr x0, x0, x1
	ret

.global __set_vector_table
__set_vector_table:
    msr vbar_el1, x0  
    ret
/*
 * void __asm_dcache_level(level)
 *
 * flush or invalidate one level cache.
 *
 * x0: cache level
 * x1: 0 clean & invalidate, 1 invalidate only, 2 clean only
 * x2~x9: clobbered
 */
.globl __asm_dcache_level
__asm_dcache_level:
    lsl    x12, x0, #1
    msr    csselr_el1, x12        /* select cache level */
    isb                /* sync change of cssidr_el1 */
    mrs    x6, ccsidr_el1        /* read the new cssidr_el1 */
    and    x2, x6, #7        /* x2 <- log2(cache line size)-4 */
    add    x2, x2, #4        /* x2 <- log2(cache line size) */
    mov    x3, #0x3ff
    and    x3, x3, x6, lsr #3    /* x3 <- max number of #ways */
    clz    w5, w3            /* bit position of #ways */
    mov    x4, #0x7fff
    and    x4, x4, x6, lsr #13    /* x4 <- max number of #sets */
    /* x12 <- cache level << 1 */
    /* x2 <- line length offset */
    /* x3 <- number of cache ways - 1 */
    /* x4 <- number of cache sets - 1 */
    /* x5 <- bit position of #ways */

loop_set:
    mov    x6, x3            /* x6 <- working copy of #ways */
loop_way:
    lsl    x7, x6, x5
    orr    x9, x12, x7        /* map way and level to cisw value */
    lsl    x7, x4, x2
    orr    x9, x9, x7        /* map set number to cisw value */
    cmp    w1, #1
    b.eq   3f
    cmp    w1, #2
    b.eq   4f
    dc    cisw, x9        /* clean & invalidate by set/way */
    b    2f
3:  dc    isw, x9         /* invalidate by set/way */
    b    2f
4:  dc    csw, x9         /* clean by set/way */
2:    subs    x6, x6, #1        /* decrement the way */
    b.ge    loop_way
    subs    x4, x4, #1        /* decrement the set */
    b.ge    loop_set

    ret

/*
 * void __asm_flush_dcache_all(int invalidate_only)
 *
 * x0: 0 clean & invalidate, 1 invalidate only
 *
 * flush or invalidate all data cache by SET/WAY.
 */
.globl __asm_dcache_all
__asm_dcache_all:
    mov    x1, x0
    dsb    sy
    mrs    x10, clidr_el1        /* read clidr_el1 */
    lsr    x11, x10, #24
    and    x11, x11, #0x7        /* x11 <- loc */
    cbz    x11, finished        /* if loc is 0, exit */
    mov    x15, lr
    mov    x0, #0            /* start flush at cache level 0 */
    /* x0  <- cache level */
    /* x10 <- clidr_el1 */
    /* x11 <- loc */
    /* x15 <- return address */

loop_level:
    lsl    x12, x0, #1
    add    x12, x12, x0        /* x0 <- tripled cache level */
    lsr    x12, x10, x12
    and    x12, x12, #7        /* x12 <- cache type */
    cmp    x12, #2
    b.lt    skip            /* skip if no cache or icache */
    bl    __asm_dcache_level    /* x1 = 0 flush, 1 invalidate */
skip:
    add    x0, x0, #1        /* increment cache level */
    cmp    x11, x0
    b.gt    loop_level

    mov    x0, #0
    msr    csselr_el1, x0        /* restore csselr_el1 */
    dsb    sy
    isb
    mov    lr, x15

finished:
    ret

.global __invalidate_icache_all
__invalidate_icache_all:
    ic      iallu                   // Invalidate the entire instruction cache
    dsb     sy                     // Data synchronization barrier
    isb                             // Synchronize the execution context
    ret

.globl __flush_dcache_all
__flush_dcache_all:
    mov    x0, #0
    b    __asm_dcache_all
    ret

.globl __invalidate_dcache_all
__invalidate_dcache_all:
    mov    x0, #1
    b    __asm_dcache_all
    ret

/* clean (write back, keep) all data cache by SET/WAY */
.globl __clean_dcache_all
__clean_dcache_all:
    mov    x0, #2
    b    __asm_dcache_all
    ret

/*
 * Page tables are walked as inner-shareable write-back memory (TCR IRGN/ORGN)
 * so a dsb is all that is needed to publish PTE stores to the walker; no
 * D-cache maintenance belongs in any of the TLB paths below.
 */
.global __flush_tlb
__flush_tlb:
    dsb ishst
    tlbi vmalle1is
	dsb ish
	isb
	ret

/* void __flush_tlb_asid(uint64_t asid): drop every entry tagged with one ASID */
.global __flush_tlb_asid
__flush_tlb_asid:
    lsl x0, x0, #48
    dsb ishst
    tlbi aside1is, x0
	dsb ish
	isb
	ret

/* local-core-only full invalidate, for the no-ASID switch fallback */
.global __flush_tlb_local
__flush_tlb_local:
    dsb nshst
    tlbi vmalle1
	dsb nsh
	isb
	ret

.global __set_translation_table_base
__set_translation_table_base:
	msr ttbr0_el1, x0
	msr ttbr1_el1, x0
	isb
	ret

/*
 * void __set_translation_table_base_asid(uint64_t base, uint64_t asid)
 * TCR.A1=0: the ASID lives in TTBR0[63:48]. Switching only the base+ASID
 * needs no TLBI because user (nG) entries of other spaces stay tagged with
 * their own ASID and the kernel half is global and identical everywhere.
 */
.global __set_translation_table_base_asid
__set_translation_table_base_asid:
	msr ttbr1_el1, x0
	orr x0, x0, x1, lsl #48
	msr ttbr0_el1, x0
	isb
	ret

/*
 * void __dcache_clean_pou_range(uint64_t start, uint64_t end)
 * Clean freshly written code to the point of unification so instruction
 * fetch (which does not snoop the L1 D-cache) sees it. Line size from CTR_EL0.
 */
.global __dcache_clean_pou_range
__dcache_clean_pou_range:
    mrs x2, ctr_el0
    ubfx x2, x2, #16, #4        /* DminLine: log2(words) */
    mov x3, #4
    lsl x3, x3, x2              /* line size in bytes */
    sub x4, x3, #1
    bic x0, x0, x4
1:  dc cvau, x0
    add x0, x0, x3
    cmp x0, x1
    b.lo 1b
    dsb ish
    ret

/* invalidate the whole I-cache on every core of the inner-shareable domain */
.global __invalidate_icache_all_is
__invalidate_icache_all_is:
    ic ialluis
    dsb ish
    isb
    ret

/*
 * void __dcache_flush_poc_range(uint64_t start, uint64_t end)
 * Clean & invalidate a VA range to the point of coherency, for data the
 * kernel wrote through a cacheable alias that a non-coherent master (GPU,
 * dma engine) or a Non-Cacheable alias will read from DRAM.
 */
.global __dcache_flush_poc_range
__dcache_flush_poc_range:
    mrs x2, ctr_el0
    ubfx x2, x2, #16, #4        /* DminLine: log2(words) */
    mov x3, #4
    lsl x3, x3, x2              /* line size in bytes */
    sub x4, x3, #1
    bic x0, x0, x4
1:  dc civac, x0
    add x0, x0, x3
    cmp x0, x1
    b.lo 1b
    dsb sy
    ret

.global __irq_enable
__irq_enable:
	msr daifclr, #0x2
	ret

.global __irq_disable
__irq_disable:
	msr daifset, #0x2
	ret

.global __int_off
__int_off:
	mrs x0, daif
	msr daifset, #0x2
	ret

.global __int_on
__int_on:
	msr daif, x0
	ret

.global __smp_lock
__smp_lock:
    ldxr    w1, [x0]        
    cbnz    w1, __lock_wait          
    mov     w1, #1         
    stxr    w2, w1, [x0]
    cbnz    w2, __smp_lock
    dmb     ish 
    ret

__lock_wait:  
	wfe                     
    b       __smp_lock

.global __smp_unlock
__smp_unlock:
    dmb     ishst           
    mov     w1, #0          
    str     w1, [x0]        
    dsb     ish             
    sev                     
    ret                     
