#define KERNEL_BASE 0x80000000

.section ".init"
.global __entry
__entry:
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

	bl _kernel_entry_c
halt:
	b halt
