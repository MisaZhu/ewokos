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

_halt:
	wfi
	b	_halt
