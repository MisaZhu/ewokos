.section ".init"
.global __entry
__entry:
	// Check for HYP mode
	mrs r0, cpsr_all
	and r0, r0, #0x1F
	mov r8, #0x1A
	cmp r0, r8
	beq over_hyped
	b continue

over_hyped: // Get out of HYP mode 
	ldr r1, =continue
	msr ELR_hyp, r1
	mrs r1, cpsr_all
	and r1, r1, #0x1f  //CPSR_MODE_MASK
	orr r1, r1, #0x13	 //CPSR_MODE_SUPERVISOR
	msr SPSR_hyp, r1
	eret

continue:
	mrc p15, 0, r11, c0, c0, 5  @ get core id
	and r11, r11, #0xff         @ 0xff cores max
	cmp r11, #0                 @ core 0, jump to core0
#ifdef KERNEL_SMP
	bne slave_core              @ init rest slave cores
#else
	bne halt                    @ halt all the rest slave cores
#endif

core0:
	//0 core
	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp,  = _svc_start_stack @ initialise SVC mode stack
	mov r0, #0xFF000000 
	and r0, r0, pc
	add sp, sp, r0

	bl flush_dcache_all
	bl cpu_mmu_disable
	bl cpu_dcache_disable
	bl cpu_icache_disable

#ifdef KERNEL_SMP
	bl arm_smp_enable
#endif

	bl _boot_start              @ enable MMU for kernel (one-level section type paging for kernel init)

	msr cpsr, #0xD2             @ enter IRQ mode with IRQ and FIQ interrupts disabled
	ldr sp, = _irq_stack        @ initialise IRQ mode stack

	msr cpsr, #0xD7             @ enter ABT mode with IRQ and FIQ interrupts disabled
	ldr sp, = _abt_stack        @ initialise ABT mode stack

	msr cpsr, #0xDB             @ enter UND mode with IRQ and FIQ interrupts disabled
	ldr sp, = _undef_stack      @ initialise UND mode stack

	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_stack        @ initialise SVC mode stack

	bl _enable_neon
	bl _kernel_entry_c
	bl halt

#ifdef KERNEL_SMP
slave_core:                     @ 1-0xff cores
	mrc p15, 0, r11, c0, c0, 5
	and r11, r11, #0xff            @ get core id (1-0xff) to r11

	mov r0, #0x1000             @ 4096 stack size for each core
	mul r0, r0, r11             @ stack offset for current core

	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_start_stack        @ initialise SVC mode stack
	sub sp, r0                  @ stack offset for current core

	mov r0, #0xFF000000 
	and r0, r0, pc
	add sp, sp, r0
	
	bl arm_smp_enable

	bl _boot_start              @ enable MMU for kernel init only

	mov r0, #0x1000
	mul r0, r0, r11             @ stack offset for current core

	msr cpsr, #0xD2             @ enter IRQ mode with IRQ and FIQ interrupts disabled
	ldr sp, = _irq_stack        @ initialise IRQ mode stack
	sub sp, r0

	msr cpsr, #0xD7             @ enter ABT mode with IRQ and FIQ interrupts disabled
	ldr sp, = _abt_stack        @ initialise ABT mode stack
	sub sp, r0

	msr cpsr, #0xDB             @ enter UND mode with IRQ and FIQ interrupts disabled
	ldr sp, = _undef_stack      @ initialise UND mode stack
	sub sp, r0

	msr cpsr, #0x13             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_stack        @ initialise SVC mode stack
	sub sp, r0
	
	bl _enable_neon
	bl _slave_kernel_entry_c
	bl halt
#endif

halt:
	b halt

.global load_boot_pgt
load_boot_pgt:
    mcr p15, 0, r0, c2, c0, 0

    ldr r1, =0x55555555
    mcr p15, 0, r1, c3, c0, 0
    dmb

    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, #0x001
    mcr p15, 0, r0, c1, c0, 0
    dsb

    mrc p15, 0, r0, c1, c0, 0
    ldr r1, =0x00003804
    orr r0, r0, r1
    mcr p15, 0, r0, c1, c0, 0

    mov r0, #0
    mcr p15, 0, r0, c8, c7, 0
    mcr p15, 0, r0, c7, c10, 4
    bx lr

v7_flush_dcache_all:
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

flush_dcache_all:
    stmfd    sp!, {r4-r5, r7, r9-r11, lr}

    bl		v7_flush_dcache_all
    ldmfd    sp!, {r4-r5, r7, r9-r11, lr}

    bx    lr

.globl arm_smp_disable
arm_smp_disable:
	mrc p15, 0, r0, c1, c0, 1   // clear SMP bit in ACTLR
	bic r0, r0, #0x40
	mcr p15, 0, r0, c1, c0, 1
	bx lr

.globl arm_smp_enable
arm_smp_enable:
	mrc p15, 0, r0, c1, c0, 1   // set SMP bit in ACTLR
	orr r0, r0, #0x40
	mcr p15, 0, r0, c1, c0, 1
	bx lr

.globl cpu_mmu_disable
cpu_mmu_disable:
	mcr     p15, #0, r0, c8, c7, #0    @ invalidate tlb
	mrc     p15, #0, r0, c1, c0, #0
	bic     r0, r0, #1
	mcr     p15, #0, r0, c1, c0, #0    @ clear mmu bit
	dsb
	bx      lr

.local cpu_dcache_disable
cpu_dcache_disable:
	push    {r4-r11, lr}
	mrc     p15, #0, r0, c1, c0, #0
	bic     r0,  r0, #0x00000004
	mcr     p15, #0, r0, c1, c0, #0
	pop     {r4-r11, lr}
	bx      lr

.local cpu_icache_disable
cpu_icache_disable:
	mrc     p15, #0, r0, c1, c0, #0
	bic     r0,  r0, #0x00001000
	mcr     p15, #0, r0, c1, c0, #0
	bx      lr
