.align 3
.extern _kernel_entry_c
.section ".init"
.align 3
.global __entry
__entry:
    # Mask exceptions and interrupts
    msr DAIFSet, #0xf
	
	# Check if we are running in EL2 and switch to EL1
    mrs x0, CurrentEL
    cmp x0, #4
    beq __el1_entry

    # Enable AARCH64 mode in EL1; Disable general exceptions trap
    mov x0, #1
    mov x1, xzr
    bfi x1, x0, #31, #1
    msr HCR_EL2, x1

    # Switch to EL1
    mov x0, #0x3c5
    msr SPSR_EL2, x0
    adr x2, __el1_entry 
    msr ELR_EL2, x2
    eret
	nop

__el1_entry:	
	/* Disable Alignment check */
	mrs     x0, sctlr_el1
	bic     x0, x0, #(3 << 3)
	bic     x0, x0, #(1 << 1)
	msr     sctlr_el1, x0

    # Disable the MMU
    mrs x0, SCTLR_EL1
    mov x1, #1
    bic x0, x0, x1
    msr SCTLR_EL1, x0

    # Enable floating point & SIMD
    mrs x0, CPACR_EL1
    mov x1, #0x3
    bfi x0, x1, #20, #2
    msr CPACR_EL1, x0

	# Enable SMP
	mrs x0, ACTLR_EL1
	orr x0, x0, #0x40
	msr ACTLR_EL1, x0

    # Set the el1 stack
    msr spsel, #0x1	
	adr x0, #_svc_start_stack
    mov sp, x0

	# Create a the first frame record, this should be 0
    mov fp, xzr
    mov lr, xzr
    stp fp, lr, [sp, #-16]!
    mov fp, sp

    # Flush and invalidate the dcache and icache and TLB
    #bl arch_dcache_flush_all
    ic ialluis
    dsb sy
    isb sy
    tlbi vmalle1is
    dsb sy
    isb sy

    # Enable the icache, dcache
    mrs x0, SCTLR_EL1
    mov x1, #0x1004
    orr x0, x0, x1
    msr SCTLR_EL1, x0

	bl _boot_start

#ifdef KERNEL_SMP
	mrs x0, mpidr_el1
    and x0, x0, #0xFF
    cmp x0, #0
    bne slave_core
#endif

	msr spsel, #0x1	
	ldr x0, =_svc_stack
	mov sp, x0

	ldr x1, =_kernel_entry_c
	br x1 
	b _halt

#ifdef KERNEL_SMP
slave_core:      
	mov x1, #0x4000
	mul x0, x0, x1
    ldr x1, =_svc_stack
	add x0, x0, x1
	msr spsel, #0x1	
	mov sp, x0

	bl _slave_kernel_entry_c
	b	halt
#endif

#define  MAIR1VAL ((0x00ul << (0 * 8)) |\
                   (0x04ul << (1 * 8)) |\
                   (0x44ul << (2 * 8)) |\
                   (0xfful << (3 * 8)) )

 #define TCREL1VAL  ((0x0ul << 37) | /* TBI=0, no tagging */\
                     (0x2ul << 32) | /* IPS= 32 bit ... 000 = 32bit, 001 = 36bit, 010 = 40bit */\
                     (0x2ul << 30) | /* TG1=4k ... options are 10=4KB, 01=16KB, 11=64KB ... take care differs from TG0 */\
                     (0x3ul << 28) | /* SH1=3 inner ... options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable */\
                     (0x1ul << 26) | /* ORGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
                     (0x1ul << 24) | /* IRGN1=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
                     (0x0ul << 23) |  /* EPD1 ... Translation table walk disable for translations using TTBR1_EL1  0 = walk, 1 = generate fault */\
                     (0x2UL << 16) | /* T1SZ=2 (2^46 size) ... The region size is 2 POWER (64-T1SZ) bytes */\
                     (0x0ul << 14) | /* TG0=4k  ... options are 00=4KB, 01=64KB, 10=16KB,  ... take care differs from TG1 */\
                     (0x3ul << 12) | /* SH0=3 inner ... .. options 00 = Non-shareable, 01 = INVALID, 10 = Outer Shareable, 11 = Inner Shareable */\
                     (0x1ul << 10) | /* ORGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
                     (0x1ul << 8)  | /* IRGN0=1 write back .. options 00 = Non-cacheable, 01 = Write back cacheable, 10 = Write thru cacheable, 11 = Write Back Non-cacheable */\
                     (0x0ul << 7) | /* EPD0  ... Translation table walk disable for translations using TTBR0_EL1  0 = walk, 1 = generate fault */\
                     (25ul  << 0) ) /* T0SZ=25 (512G)  ... The region size is 2 POWER (64-T0SZ) bytes */

 #define SCTLREL1VAL ((0xC00800)| /* set mandatory reserved bits */\
                      (1 << 12) | /* I, Instruction cache enable. This is an enable bit for instruction caches at EL0 and EL1 */\
                      (0 << 4)  | /* SA0, tack Alignment Check Enable for EL0 */\
                      (0 << 3)  | /* SA, Stack Alignment Check Enable */\
                      (1 << 2)  | /* C, Data cache enable. This is an enable bit for data caches at EL0 and EL1 */\
                      (0 << 1)  | /* A, Alignment check enable bit */\
                      (1 << 0))   /* set M, enable MMU */


.global load_boot_pgt
load_boot_pgt:
    ldr x1, =MAIR1VAL
	msr mair_el1, x1

    msr ttbr0_el1, x0 
	isb

    ldr x0, =TCREL1VAL
    msr tcr_el1, x0
    isb sy
    dsb sy

    mrs x0, sctlr_el1
    ldr x1, =SCTLREL1VAL
    orr x0, x0, x1
    msr sctlr_el1, x0

    dsb sy
    isb sy
	ret

_halt:
	wfi
	b	_halt
