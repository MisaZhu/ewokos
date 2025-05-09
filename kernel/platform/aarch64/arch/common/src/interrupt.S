.macro DEBUG  char
	ldr x28, =0xE8215040
	mov x29, \char
	str x29, [x28]
.endm

.macro SAVE_IRQ_CONTEXT
    /* Save the entire context. */
	stp		q30, q31, [sp, #-32]!
	stp 	q28, q29, [sp, #-32]!
	stp 	q26, q27, [sp, #-32]!
	stp 	q24, q25, [sp, #-32]!
	stp 	q22, q23, [sp, #-32]!
	stp 	q20, q21, [sp, #-32]!
	stp 	q18, q19, [sp, #-32]!
	stp 	q16, q17, [sp, #-32]!
	stp 	q14, q15, [sp, #-32]!
	stp 	q12, q13, [sp, #-32]!
	stp 	q10, q11, [sp, #-32]!
	stp 	q8,  q9,  [sp, #-32]!
	stp 	q6,  q7,  [sp, #-32]!
	stp 	q4,  q5,  [sp, #-32]!
	stp 	q2,  q3,  [sp, #-32]!
	stp 	q0,  q1,  [sp, #-32]!

    stp     x28, x29, [sp, #-16]!
    stp     x26, x27, [sp, #-16]!
    stp     x24, x25, [sp, #-16]!
    stp     x22, x23, [sp, #-16]!
    stp     x20, x21, [sp, #-16]!
    stp     x18, x19, [sp, #-16]!
    stp     x16, x17, [sp, #-16]!
    stp     x14, x15, [sp, #-16]!
    stp     x12, x13, [sp, #-16]!
    stp     x10, x11, [sp, #-16]!
    stp     x8, x9, [sp, #-16]!
    stp     x6, x7, [sp, #-16]!
    stp     x4, x5, [sp, #-16]!
    stp     x2, x3, [sp, #-16]!
    stp     x0, x1, [sp, #-16]!

    mrs     x29, fpsr
    mrs     x28, fpcr
    stp     x28, x29, [sp, #-16]!

    mrs     x29, sp_el0
    stp     x29, x30, [sp, #-16]!

    mrs     x29, spsr_el1
    mrs     x28, elr_el1
    stp     x28, x29, [sp, #-16]!
.endm


.macro RESTORE_IRQ_CONTEXT
    ldp     x28, x29, [sp], #16  /* SPSR and ELR. */
    tst     x29, #0x1f
    msr     spsr_el1, x29
    msr     elr_el1, x28

    ldp     x29, x30, [sp], #16
    msr     sp_el0, x29

    ldp     x28, x29, [sp], #16
    msr     fpcr, x28
    msr     fpsr, x29

    ldp     x0, x1, [sp], #16
    ldp     x2, x3, [sp], #16
    ldp     x4, x5, [sp], #16
    ldp     x6, x7, [sp], #16
    ldp     x8, x9, [sp], #16
    ldp     x10, x11, [sp], #16
    ldp     x12, x13, [sp], #16
    ldp     x14, x15, [sp], #16
    ldp     x16, x17, [sp], #16
    ldp     x18, x19, [sp], #16
    ldp     x20, x21, [sp], #16
    ldp     x22, x23, [sp], #16
    ldp     x24, x25, [sp], #16
    ldp     x26, x27, [sp], #16
    ldp     x28, x29, [sp], #16

	ldp     q0,	 q1,  [sp], #32
    ldp     q2,  q3,  [sp], #32
    ldp     q4,  q5,  [sp], #32
    ldp     q6,  q7,  [sp], #32
    ldp     q8,  q9,  [sp], #32
    ldp     q10, q11, [sp], #32
    ldp     q12, q13, [sp], #32
    ldp     q14, q15, [sp], #32
    ldp     q16, q17, [sp], #32
    ldp     q18, q19, [sp], #32
    ldp     q20, q21, [sp], #32
    ldp     q22, q23, [sp], #32
    ldp     q24, q25, [sp], #32
    ldp     q26, q27, [sp], #32
    ldp     q28, q29, [sp], #32
    ldp     q28, q29, [sp], #32

.endm

.global interrupt_table_start, interrupt_table_end
.balign 0x800
interrupt_table_start: /*interrupt table, with syscall and irq items*/
/*Synchronous*/
    DEBUG #0x30
	b _undef_exception_entry
.balign 0x80	/*IRQ*/ 
    DEBUG #0x31
	b _undef_exception_entry
.balign 0x80	/*FIQ*/
    DEBUG #0x32
	b _undef_exception_entry
.balign 0x80	/*sError*/
    DEBUG #0x33
	b _undef_exception_entry
.balign 0x80	/*Synchronous*/
    #DEBUG #0x34
	b sync_entry 
.balign 0x80	/*IRQ*/
    #DEBUG #0x35
	b irq_entry
.balign 0x80    /*FIQ*/
    DEBUG #0x36
	b _undef_exception_entry
.balign 0x80	/*sError*/
    DEBUG #0x37
	b _undef_exception_entry
.balign 0x80	 /*Synchronous*/
    #DEBUG #0x38
	b	sync_entry 
.balign 0x80   /*IRQ*/
    #DEBUG #0x39
	b irq_entry
.balign 0x80	/*FIQ*/
    DEBUG #0x3A
	b _undef_exception_entry
.balign 0x80	/*sError*/
    DEBUG #0x3B
	b	_undef_exception_entry
.balign 0x80	 /*Synchronous*/
    DEBUG #0x3C
	b _undef_exception_entry
.balign 0x80   /*IRQ*/
    #DEBUG #0x3D
	b irq_entry
.balign 0x80	/*FIQ*/
    DEBUG #0x3E
	b _undef_exception_entry
.balign 0x80	/*sError*/
    DEBUG #0x3F
	b _undef_exception_entry 

interrupt_table_end:
.balign 0x80

sync_entry:
	SAVE_IRQ_CONTEXT
	mrs x9, esr_el1
	and x9, x9, #0xFC000000
	mov x10, #0x54000000
	cmp x9, x10
	beq svc_entry
	mrs x0, esr_el1
	mrs x1, far_el1
	mov x2, sp
	bl sync_exception_handle
	RESTORE_IRQ_CONTEXT
	eret
svc_entry:
	RESTORE_IRQ_CONTEXT
	SAVE_IRQ_CONTEXT
	mov x4, sp
	bl svc_handler
	RESTORE_IRQ_CONTEXT
	eret

irq_entry:
	SAVE_IRQ_CONTEXT
	#mov x0, sp
	#bl dump_ctx
	mov x0, sp
	bl irq_handler
	#mov x0, sp
	#bl dump_ctx
	RESTORE_IRQ_CONTEXT
	eret

fiq_entry:
serror_entry:
_undef_exception_entry:
	SAVE_IRQ_CONTEXT
	mov x0, sp
	bl dump_ctx 
_halt:
	b _halt 
