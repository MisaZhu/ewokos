#include <mmu.h>

.global __entry
__entry:
	ldr sp, =_initStack
	add sp, #4096

	bl enablePaging
	bl __jump2HighMem

	bl _kernelEntry

enablePaging:
	mov r2, lr

	mov r0, #1
	bl __setDomainAccessControl

	ldr r0, =_startupSectionTable
	sub r0, #KERNEL_BASE
	bl __setTranslationTableBase

	bl __readControlRegister
	orr r0, #1
	bl __setControlRegister

	mov pc, r2

