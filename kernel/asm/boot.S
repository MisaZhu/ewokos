#include <mm/mmu.h>
#include <proc.h>

.global __entry
__entry:
	ldr sp, =_initStack /*sp(R13), _initStack defined in kernel/src/mm/startup.c*/
	add sp, #4096

	bl enablePaging
	bl copyInterruptTable
	bl __useHighInterrupts  /* (system.S*/
	bl __enableInterrupts /*system.S*/
	bl __jump2HighMem /*system.S*/

	bl kernelEntry /*C entry at kernel/src/kernel.c*/

/*set up the init page table for kernel boot and init*/
enablePaging:
	mov r2, lr /*lr(R14) return PC stored*/

	mov r0, #1
	bl __setDomainAccessControl /*system.S*/

	ldr r0, =_startupPageDir /*kernel/src/mm/startup.c*/
	sub r0, #KERNEL_BASE /*get physical address from virtual address*/
	bl __setTranslationTableBase /*system.S*/

	bl __readControlRegister /*system.S*/
	orr r0, #1 /*enable paging*/
	bl __setControlRegister /*system.S*/

	mov pc, r2 /*return*/

/*Copy interrupt talbe to phymen address 0x00000000.
Virtual address #INTERRUPT_VECTOR_BASE(0xFFFF0000 for ARM) must mapped to phymen 0x00000000.
ref: setKernelVM(PageDirEntryT* vm) kernel/src/kernel.c 
*/
copyInterruptTable: 
	mov r0, #0x0
	ldr r1, =interruptTableStart
	ldr r3, =interruptTableEnd
	
	copyLoopStart:
		ldr r2, [r1, #0x0] /*read from interruptTable to R2*/
		str r2, [r0, #0x0] /*write R2 to phy address(r0)*/
		add r0, r0, #0x4 /*move to next phy address(32bits)*/
		add r1, r1, #0x4 /*move to next table item*/
		cmp r1, r3 /*if endof table*/
		bne copyLoopStart

	mov pc, lr

interruptTableStart: /*interrupt table, with syscall and irq items*/
	nop /*item 0: reset*/
	ldr pc, abortEntryAddress /*item 1: undef instruct*/
	ldr pc, syscallEntryAddress /*item 2: syscall*/
	ldr pc, abortEntryAddress /*item 3: prefetchAbort*/
	ldr pc, abortEntryAddress /*item 4: dataAbort*/
	subs pc, lr, #4 /*item 5: return*/
	ldr pc, irqEntryAddress /*item 6: irq*/
	
	syscallEntryAddress: .word syscallEntry
	irqEntryAddress: .word irqEntry
	abortEntryAddress: .word abortEntry
interruptTableEnd:

syscallEntry:
	ldr sp, =_initStack /*kernel/src/mm/startup.c*/
	add sp, #4096

	SAVE_CONTEXT /*proc.h*/

	stmfd sp!, {r1-r12, r14} /*push r0-r12, r14(return pc) to stack*/

	bl handleSyscall /*kernel/src/syscalls.c*/

	ldmfd sp!, {r1-r12, pc}^ /*pop stack to r0-r12, pc(means return)*/

irqEntry:
	sub r14, r14, #4
	ldr sp, =_irqStack
	add sp, #4096

	SAVE_CONTEXT

	stmfd sp!, {r0-r12, r14} /*push r0-r12, r14(return pc) to stack*/

	bl handleIRQ /*kernel/src/irq.c*/

	ldmfd sp!, {r0-r12, pc}^ /*pop stack to r0-r12, pc(means return)*/

abortEntry:
	# Abort entry.  Switch back to Supervisor mode, and transfer
	# to C abort handler
	mrs r2, cpsr
	bic r2, #0xf /*clear lower 4 bits(CPSR-MODE) to 0*/
	orr r2, #0x3 /*set lower 4 bits(CPSR-MODE) to 0x3, means svc mode*/
	msr cpsr, r2
	bl _abortEntry /*kernel/src/proc.c*/
