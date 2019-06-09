#include <mm/mmu.h>
#include <proc.h>

.global __entry
__entry:
#ifdef CPU_NUM
	mrc p15, #0, r11, c0, c0, #5
	and r11, r11, #3
	LDR r0, = _init_stack //sp(R13), _init_stack defined in kernel/src/mm/startup.c
	mov r1, r11 // r1 = cpuid
	add r1, r1, #1
	lsl r2, r1, #12 // cpuid++ // (cpuid+1)* 4096
	add r0, r0, r2
	mov sp, r0 // SVC sp=svc_stack[cpuid] high end

	MSR cpsr, #0x92 //irq mode
	LDR r0, = _irq_stack //sp(R13), _irq_stack defined in kernel/src/mm/startup.c
	mov r1, r11 // r1 = cpuid
	add r1, r1, #1
	lsl r2, r1, #12 // cpuid++ // (cpuid+1)* 4096
	add r0, r0, r2
	mov sp, r0 // IRQ sp=irq_stack[cpuid] high end
#else
	ldr sp, =_init_stack
	add sp, #(4096*4)

	MSR cpsr, #0x92 //irq mode
	ldr sp, =_irq_stack
	add sp, #(4096*4)
#endif
	MSR cpsr, #0x13 //svc mode

	bl enable_paging
	bl __jump2_high_mem /*system.S*/

#ifdef CPU_NUM
	cmp r11, #0
	bne __APS
#endif

	bl copy_interrupt_table
	bl __use_high_interrupts  /* (system.S*/
	bl __enable_interrupts /*system.S*/
	bl kernel_entry /*C entry at kernel/src/kernel.c*/
	b .

#ifdef CPU_NUM
__APS:
	bl kernel_entry /*C entry at kernel/src/kernel.c*/
	b .
#endif

/*set up the init page table for kernel boot and init*/
enable_paging:
	mov r2, lr /*lr(R14) return PC stored*/

	mov r0, #1
	bl __set_domain_access_control /*system.S*/

	ldr r0, =_startup_page_dir /*kernel/src/mm/startup.c*/
	sub r0, #KERNEL_BASE /*get physical address from virtual address*/
	bl __set_translation_table_base /*system.S*/

	bl __read_control_register /*system.S*/
	orr r0, #1 /*enable paging*/
	bl __set_control_register /*system.S*/

	mov pc, r2 /*return*/

/*Copy interrupt talbe to phymen address 0x00000000.
Virtual address #INTERRUPT_VECTOR_BASE(0xFFFF0000 for ARM) must mapped to phymen 0x00000000.
ref: set_kernel_vm(page_dir_entry_t* vm) kernel/src/kernel.c 
*/
copy_interrupt_table: 
	mov r0, #0x0
	ldr r1, =interrupt_table_start
	ldr r3, =interrupt_table_end
	
	copy_loop_start:
		ldr r2, [r1, #0x0] /*read from interrupt_table to R2*/
		str r2, [r0, #0x0] /*write R2 to phy address(r0)*/
		add r0, r0, #0x4 /*move to next phy address(32bits)*/
		add r1, r1, #0x4 /*move to next table item*/
		cmp r1, r3 /*if endof table*/
		bne copy_loop_start

	mov pc, lr

interrupt_table_start: /*interrupt table, with syscall and irq items*/
	nop /*item 0: reserved for reset*/
	ldr pc, abort_entry_address /*item 1: undef instruct*/
	ldr pc, syscall_entry_address /*item 2: syscall*/
	ldr pc, abort_entry_address /*item 3: prefetchAbort*/
	ldr pc, abort_entry_address /*item 4: dataAbort*/
	ldr pc, abort_entry_address /*item 5: dataAbort*/
	//nop /*item 5: reserved*/
	ldr pc, irq_entry_address /*item 6: irq*/
	
	syscall_entry_address: .word syscall_entry
	irq_entry_address: .word irq_entry
	abort_entry_address: .word abort_entry
interrupt_table_end:

syscall_entry:
#ifndef CPU_NUM
	ldr sp, =_init_stack //sp(R13), _init_stack defined in kernel/src/mm/startup.c
	add sp, #(4096*4)
#endif

	SAVE_CONTEXT /*save current process content, kernel/include/proc.h*/

	stmfd sp!, {r1-r12, r14} /*push r1-r12, r14(return pc) to stack*/

	bl handle_syscall /*kernel/src/syscalls.c*/

	ldmfd sp!, {r1-r12, pc}^ /*pop stack to r1-r12, pc(means return)*/

irq_entry:
#ifndef CPU_NUM
	ldr sp, =_irq_stack //sp(R13), _irq_stack defined in kernel/src/mm/startup.c
	add sp, #(4096*4)
#endif

	sub r14, r14, #4
	SAVE_CONTEXT

	stmfd sp!, {r0-r12, r14} /*push r0-r12, r14(return pc) to stack*/

	bl irq_handle /*arch/{ARCH}/src/irq.c*/

	ldmfd sp!, {r0-r12, pc}^ /*pop stack to r0-r12, pc(means return)*/

abort_entry:
	/*
	sub r14, r14, #8
	ldr sp, =_abort_stack
	add sp, #4096
	stmfd sp!, {r0-r12, r14} //push r0-r12, r14(return pc) to stack
	mov r0, r3
	bl _abort_entry
	ldmfd sp!, {r0-r12, pc}^ //pop stack to r0-r12, pc(means return)
	*/
	# Abort entry.  Switch back to Supervisor mode, and transfer
	# to C abort handler
	mrs r2, cpsr
	bic r2, #0xf //clear lower 4 bits(CPSR-MODE) to 0
	orr r2, #0x3 //set lower 4 bits(CPSR-MODE) to 0x3, means svc mode
	msr cpsr, r2
	mov r0, r3
	bl _abort_entry //kernel/src/proc.c

.global __halt
__halt:
	b __halt
