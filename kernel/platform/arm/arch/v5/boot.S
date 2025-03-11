
.section ".init"
.global __entry
__entry:
	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_start_stack        @ initialise SVC mode stack
	mov r0, #0xFF000000
    and r0, r0, pc
    add sp, sp, r0
	bl _boot_start              @ MMU pagetable setup for kernel

	msr cpsr, #0xD2             @ enter IRQ mode with IRQ and FIQ interrupts disabled
	ldr sp, = _irq_stack        @ initialise IRQ mode stack

	msr cpsr, #0xD7             @ enter ABT mode with IRQ and FIQ interrupts disabled
	ldr sp, = _abt_stack        @ initialise ABT mode stack

	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_stack        @ initialise SVC mode stack

	bl _kernel_entry_c
halt:
	b halt
