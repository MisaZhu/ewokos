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
	and r11, r11, #3            @ 4 cores max
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

	bl _kernel_entry_c
	bl halt

#ifdef KERNEL_SMP
slave_core:                     @ 1-3 cores
	mrc p15, 0, r11, c0, c0, 5
	and r11, r11, #3            @ get core id (1-3) to r11

	mov r0, #0x1000             @ 4096 stack size for each core
	mul r0, r0, r11             @ stack offset for current core

	msr cpsr, #0xD3             @ enter SVC mode with IRQ and FIQ interrupts disabled
	ldr sp, = _svc_start_stack        @ initialise SVC mode stack
	sub sp, r0                  @ stack offset for current core

	mov r0, #0xFF000000 
	and r0, r0, pc
	add sp, sp, r0
	
	bl arm_smp_enable

	bl _boot_start              @ enable MMU for kernel

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
	
	bl _slave_kernel_entry_c
	bl halt
#endif

halt:
	b halt

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

kernel_base: .word 0xffff0000
