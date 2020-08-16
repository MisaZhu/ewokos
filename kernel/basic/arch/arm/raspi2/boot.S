#define KERNEL_BASE 0x80000000

.section ".init"
.global __entry
__entry:
/*
	//==================================================================
	// Disable caches, MMU and branch prediction in case they were left enabled from an earlier run
	// This does not need to be done from a cold reset
	//==================================================================

	MRC     p15, 0, r0, c1, c0, 0       // Read CP15 System Control register
	BIC     r0, r0, #(0x1 << 12)        // Clear I bit 12 to disable I Cache
	BIC     r0, r0, #(0x1 <<  2)        // Clear C bit  2 to disable D Cache
	BIC     r0, r0, #0x1                // Clear M bit  0 to disable MMU
	BIC     r0, r0, #(0x1 << 11)        // Clear Z bit 11 to disable branch prediction
	MCR     p15, 0, r0, c1, c0, 0       // Write value back to CP15 System Control register

	// The MMU is enabled later, before calling main().  Caches and branch prediction are enabled inside main(),
	// after the MMU has been enabled and scatterloading has been performed.

	//===================================================================
	// ACTLR.SMP Enables coherent requests to the processor.
	// You must ensure this bit is set to 1 before the caches and MMU are enabled, or any cache and TLB maintenance operations are performed.
	//===================================================================
	MRC     p15, 0, r0, c1, c0, 1    // Read CP15 ACTLR
	ORR     r0, r0, #(1 << 6)     // set ACTLR.SMP bit
	MCR     p15, 0, r0, c1, c0, 1    // Write CP15 ACTLR

	//==================================================================
	// Invalidate Data and Instruction TLBs and branch predictor in case they were left enabled from an earlier run
	// This does not need to be done from a cold reset
	//==================================================================

	MOV     r0,#0
	MCR     p15, 0, r0, c8, c7, 0      // I-TLB and D-TLB invalidation
	MCR     p15, 0, r0, c7, c5, 6      // BPIALL - Invalidate entire branch predictor array

	//==================================================================
	// Cache Invalidation code for Cortex-A7
	// NOTE: Neither Caches, nor MMU, nor BTB need post-reset invalidation on Cortex-A7,
	// but forcing a cache invalidation, makes the code more portable to other CPUs (e.g. Cortex-A9)
	//==================================================================
	// Invalidate L1 Instruction Cache
	MRC     p15, 1, r0, c0, c0, 1      // Read Cache Level ID Register (CLIDR)
	TST     r0, #0x3                   // Harvard Cache?
	MOV     r0, #0                     // SBZ
	MCRNE   p15, 0, r0, c7, c5, 0      // ICIALLU - Invalidate instruction cache and flush branch target cache

	// Invalidate Data/Unified Caches
	MRC     p15, 1, r0, c0, c0, 1      // Read CLIDR
	ANDS    r3, r0, #0x07000000        // Extract coherency level
	MOV     r3, r3, LSR #23            // Total cache levels << 1
	BEQ     Finished                   // If 0, no need to clean

	MOV     r10, #0                    // R10 holds current cache level << 1
Loop1:
	ADD     r2, r10, r10, LSR #1       // R2 holds cache "Set" position
	MOV     r1, r0, LSR r2             // Bottom 3 bits are the Cache-type for this level
	AND     r1, r1, #7                 // Isolate those lower 3 bits
	CMP     r1, #2
	BLT     Skip                       // No cache or only instruction cache at this level

	MCR     p15, 2, r10, c0, c0, 0     // Write the Cache Size selection register
	ISB                                // ISB to sync the change to the CacheSizeID reg
	MRC     p15, 1, r1, c0, c0, 0      // Reads current Cache Size ID register
	AND     r2, r1, #7                 // Extract the line length field
	ADD     r2, r2, #4                 // Add 4 for the line length offset (log2 16 bytes)
	LDR     r4, =0x3FF
	ANDS    r4, r4, r1, LSR #3         // R4 is the max number on the way size (right aligned)
	CLZ     r5, r4                     // R5 is the bit position of the way size increment
	LDR     r7, =0x7FFF
	ANDS    r7, r7, r1, LSR #13        // R7 is the max number of the index size (right aligned)

Loop2:
	MOV     r9, r4                     // R9 working copy of the max way size (right aligned)

Loop3:
	ORR     r11, r10, r9, LSL r5 // Factor in the Way number and cache number into R11
	ORR     r11, r11, r7, LSL r2       // Factor in the Set number
	MCR     p15, 0, r11, c7, c6, 2     // Invalidate by Set/Way
	SUBS    r9, r9, #1                 // Decrement the Way number
	BGE     Loop3
	SUBS    r7, r7, #1                 // Decrement the Set number
	BGE     Loop2

Skip:
	ADD     r10, r10, #2               // increment the cache number
	CMP     r3, r10
	BGT     Loop1

Finished:

	// Cortex-A7 supports two translation tables
	// Configure translation table base (TTB) control register cp15,c2
	// to a value of all zeros, indicates we are using TTB register 0.
	MOV     r0,#0x0
	MCR     p15, 0, r0, c2, c0, 2
*/

	// Shut off extra cores
	mrc p15, 0, r5, c0, c0, 5
	and r5, r5, #3
	cmp r5, #0
	bne halt

	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_stack        @ initialise SVC mode stack
	sub sp, #KERNEL_BASE
	bl _boot_start

	msr cpsr, #0xD2             @ enter IRQ mode with IRQ and FIQ interrupts disabled
	ldr sp, = _irq_stack        @ initialise IRQ mode stack

	msr cpsr, #0xD7             @ enter ABT mode with IRQ and FIQ interrupts disabled
	ldr sp, = _abt_stack        @ initialise ABT mode stack

	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_stack        @ initialise SVC mode stack

	sub   sp, sp, #68             @ initialise dummy context
	mov   r0, sp                  @ set    high-level C function arg. = SP

	bl _kernel_entry_c

	ldmia sp!, { r0, lr }         @ load   USR mode PC and CPSR
	msr   spsr, r0                @ set    USR mode        CPSR
	ldmia sp, { r0-r12, sp, lr }^ @ load   USR mode registers
	add   sp, sp, #60             @ update SVC mode SP
	movs  pc, lr                  @ return from interrupt

halt:
	b halt
