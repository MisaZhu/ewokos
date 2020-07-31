#define KERNEL_BASE 0x80000000

.global __entry
__entry:
	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_stack        @ initialise SVC mode stack
	sub sp, #KERNEL_BASE

	//enable paging
	mov r0, #0x1
	//set_domain_access_control
	mcr p15, 0, r0, cr3, cr0, 0

	//set_translation_table_base 
	ldr r0, =_startup_page_dir /*kernel/src/mm/startup.c*/
	sub r0, #KERNEL_BASE /*get physical address from virtual address*/
	mcr p15, 0, r0, c2, c0, 0 //set ttbase
	mcr p15, 0, r0, c2, c0, 1 //set ttbase
	mov r0, #0
	mcr p15, 0, r0, c8, c7, 0 //flush tlb
  mcr p15, 0, r0, c7, c10, 4 // DSB ??

	//read_control_register
	mrc p15, 0, r0, cr1, cr0, 0 /*operate-0 from co-processor-p15 to R0, cr1 cr0 are registers on co-proc*/
	orr r0, #0x1 
	//set_control_register
	mcr p15, 0, r0, cr1, cr0, 0 /*operate-0 from R0 to co-processor-p15*/

	//jump to high mem
	add pc, #KERNEL_BASE

	bl _boot_init

	msr cpsr, #0xD2             @ enter IRQ mode with IRQ and FIQ interrupts disabled
	ldr sp, = _irq_stack        @ initialise IRQ mode stack

	msr cpsr, #0xD7             @ enter ABT mode with IRQ and FIQ interrupts disabled
	ldr sp, = _abt_stack        @ initialise ABT mode stack

	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_stack        @ initialise SVC mode stack

	bl _copy_interrupt_table
	bl __use_high_interrupts  /* (system.S*/

	sub   sp, sp, #68             @ initialise dummy context
	mov   r0, sp                  @ set    high-level C function arg. = SP

	bl _kernel_entry_c

	ldmia sp!, { r0, lr }         @ load   USR mode PC and CPSR
	msr   spsr, r0                @ set    USR mode        CPSR
	ldmia sp, { r0-r12, sp, lr }^ @ load   USR mode registers
	add   sp, sp, #60             @ update SVC mode SP
	movs  pc, lr                  @ return from interrupt

halt:
	wfe
	b halt
