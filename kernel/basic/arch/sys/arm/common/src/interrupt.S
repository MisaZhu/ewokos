.global interrupt_table_start, interrupt_table_end
interrupt_table_start: /*interrupt table, with syscall and irq items*/
	ldr pc, reset_entry_address /*item 0: for reset*/
	ldr pc, undef_abort_entry_address /*item 1: undef instruct*/
	ldr pc, svc_entry_address /*item 2: syscall*/
	ldr pc, prefetch_abort_entry_address /*item 3: prefetchAbort*/
	ldr pc, data_abort_entry_address /*item 4: dataAbort*/
	b . /*item 5: reserved*/
	ldr pc, irq_entry_address /*item 6: irq*/

	reset_entry_address: .word __entry
	undef_abort_entry_address: .word undef_abort_entry
	svc_entry_address: .word svc_entry
	irq_entry_address: .word irq_entry
	prefetch_abort_entry_address: .word prefetch_abort_entry
	data_abort_entry_address: .word data_abort_entry
interrupt_table_end:

svc_entry:
	sub   lr, lr, #0              @ correct return address
	sub   sp, sp, #60             @ update SVC mode stack
	stmia sp, { r0-r12, sp, lr }^ @ store  USR registers
	mrs   r5, spsr                @ get    USR        CPSR
	stmdb sp!, { r5, lr }         @ store  USR PC and CPSR
	msr   cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	mov   r4, sp                  @ r4 is now the context struct address

	stmdb sp!, {r4}
	bl    svc_handler             @ invoke high-level C function
	ldmia sp!, {r4}

	ldmia sp!, { r1, lr }         @ load   USR mode PC and CPSR
	msr   spsr, r1                @ set    USR mode        CPSR
	ldmia sp, { r0-r12, sp, lr }^ @ load   USR mode registers
	add   sp, sp, #60
	movs  pc, lr                  @ return from interrupt

irq_entry:
	sub   lr, lr, #4              @ correct return address
	msr   cpsr, #0xD2             @ enter IRQ mode with IRQ and FIQ interrupts disabled
	sub   sp, sp, #60             @ update SVC mode stack
	stmia sp, { r0-r12, sp, lr }^ @ store  USR registers
	mrs   r0, spsr                @ get    USR        CPSR
	stmdb sp!, { r0, lr }         @ store  USR PC and CPSR
	mov   r0, sp                  @ r0 is now the context struct address

	bl    irq_handler             @ invoke high-level C function

	ldmia sp!, { r0, lr }         @ load   USR mode PC and CPSR
	msr   spsr, r0                @ set    USR mode        CPSR
	ldmia sp, { r0-r12, sp, lr }^ @ load   USR mode registers
	add   sp, sp, #60             @ update SVC mode SP
	movs  pc, lr                  @ return from interrupt

undef_abort_entry:
	sub   lr, lr, #4              @ correct return address
	msr   cpsr, #0xD7             @ enter ABT mode with IRQ and FIQ interrupts disabled
	sub   sp, sp, #60             @ update SVC mode stack
	stmia sp, { r0-r12, sp, lr }^ @ store  USR registers
	mrs   r0, spsr                @ get    USR        CPSR
	stmdb sp!, { r0, lr }         @ store  USR PC and CPSR
	mov   r0, sp                  @ r0 is now the context struct address
	mrc   p15, #0, r1, c5, c0, #1 @ read fault status code

	bl    undef_abort_handler  @ invoke high-level C function

	ldmia sp!, { r0, lr }         @ load   USR mode PC and CPSR
	msr   spsr, r0                @ set    USR mode        CPSR
	ldmia sp, { r0-r12, sp, lr }^ @ load   USR mode registers
	add   sp, sp, #60             @ update SVC mode SP
	movs  pc, lr                  @ return from interrupt


prefetch_abort_entry:
	sub   lr, lr, #4              @ correct return address
	msr   cpsr, #0xD7             @ enter ABT mode with IRQ and FIQ interrupts disabled
	sub   sp, sp, #60             @ update SVC mode stack
	stmia sp, { r0-r12, sp, lr }^ @ store  USR registers
	mrs   r0, spsr                @ get    USR        CPSR
	stmdb sp!, { r0, lr }         @ store  USR PC and CPSR
	mov   r0, sp                  @ r0 is now the context struct address
	mrc   p15, #0, r1, c5, c0, #1 @ read fault status code

	bl    prefetch_abort_handler  @ invoke high-level C function

	ldmia sp!, { r0, lr }         @ load   USR mode PC and CPSR
	msr   spsr, r0                @ set    USR mode        CPSR
	ldmia sp, { r0-r12, sp, lr }^ @ load   USR mode registers
	add   sp, sp, #60             @ update SVC mode SP
	movs  pc, lr                  @ return from interrupt

data_abort_entry:
	sub   lr, lr, #8              @ correct return address
	msr   cpsr, #0xD7             @ enter ABT mode with IRQ and FIQ interrupts disabled
	sub   sp, sp, #60             @ update SVC mode stack
	stmia sp, { r0-r12, sp, lr }^ @ store  USR registers
	mrs   r0, spsr                @ get    USR        CPSR
	stmdb sp!, { r0, lr }         @ store  USR PC and CPSR
	mov   r0, sp
	mrc   p15, #0, r1, c6, c0, #0 @read fault address
	mrc   p15, #0, r2, c5, c0, #0 @read fault status code

	bl    data_abort_handler      @ invoke high-level C function

	ldmia sp!, { r0, lr }         @ load   USR mode PC and CPSR
	msr   spsr, r0                @ set    USR mode        CPSR
	ldmia sp, { r0-r12, sp, lr }^ @ load   USR mode registers
	add   sp, sp, #60             @ update SVC mode SP
	movs  pc, lr                  @ return from interrupt

