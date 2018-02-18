#include <mmu.h>

.global __readControlRegister
__readControlRegister:
	mrc p15, 0, r0, cr1, cr0, 0
	mov pc, lr

.global __setControlRegister
__setControlRegister:
	mcr p15, 0, r0, cr1, cr0, 0
	mov pc, lr

.global __setDomainAccessControl
__setDomainAccessControl:
	mcr p15, 0, r0, cr3, cr0, 0
	mov pc, lr

.global __setTranslationTableBase
__setTranslationTableBase:
	mcr p15, 0, r0, cr2, cr0, 0
	mov pc, lr

.global __jump2HighMem
__jump2HighMem:
	add lr, #KERNEL_BASE
	mov pc, lr
