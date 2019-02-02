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
		ldr r2, [r1, #0x0]
		str r2, [r0, #0x0]
		add r0, r0, #0x4
		add r1, r1, #0x4
		cmp r1, r3
		bne copyLoopStart

	mov pc, lr

interruptTableStart: /*interrupt table, with syscall and irq items*/
	nop /*reset*/
	ldr pc, abortEntryAddress /*undef instruct*/
	ldr pc, syscallEntryAddress
	ldr pc, abortEntryAddress /*prefetchAbort*/
	ldr pc, abortEntryAddress /*dataAbort*/
	subs pc, lr, #4
	ldr pc, irqEntryAddress
	syscallEntryAddress: .word syscallEntry
	irqEntryAddress: .word irqEntry
	abortEntryAddress: .word abortEntry
interruptTableEnd:

syscallEntry:
	ldr sp, =_initStack /*kernel/src/mm/startup.c*/
	add sp, #4096

	SAVE_CONTEXT /*proc.h*/

	stmfd r13!, {r1-r12, r14}

	bl handleSyscall /*kernel/src/syscalls.c*/

	ldmfd r13!, {r1-r12, pc}^

irqEntry:
	sub r14, r14, #4
	ldr sp, =_irqStack
	add sp, #4096

	SAVE_CONTEXT

	stmfd r13!, {r0-r12, r14}

	bl handleIRQ /*kernel/src/irq.c*/

	ldmfd r13!, {r0-r12, pc}^

abortEntry:
	# Abort entry.  Switch back to Supervisor mode, and transfer
	# to C abort handler
	mrs r2, cpsr
	bic r2, #0xf
	orr r2, #0x3
	msr cpsr, r2
	bl _abortEntry /*kernel/src/proc.c*/
