#ifndef __ARM_ASSEMBLER_H_INCLUDE__
#define __ARM_ASSEMBLER_H_INCLUDE__

#ifdef CONFIG_ARM_THUMB2
	.macro	setmode, mode, reg
	mov	\reg, #\mode
	msr	cpsr_c, \reg
	.endm
#else
	.macro	setmode, mode, reg
	msr	cpsr_c, #\mode
	.endm
#endif

#endif /* __ARM_ASSEMBLER_H_INCLUDE__ */
