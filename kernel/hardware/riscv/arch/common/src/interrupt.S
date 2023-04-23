    .text
    .altmacro

/*
 * Prepares to enter a system call or exception by saving all registers to the
 * stack.
 */
	.macro SAVE_ALL
    //switch to kernel irq static
    csrrw sp, sscratch, sp
    addi sp, sp, -36 * 8

    //save geniric register
    sd x1,   1 * 8(sp)
    sd x3,   3 * 8(sp)
    sd x4,   4 * 8(sp)
    sd x5,   5 * 8(sp)
    sd x6,   6 * 8(sp)
    sd x7,   7 * 8(sp)
    sd x8,   8 * 8(sp)
    sd x9,   9 * 8(sp)
    sd x10, 10 * 8(sp)
    sd x11, 11 * 8(sp)
    sd x12, 12 * 8(sp)
    sd x13, 13 * 8(sp)
    sd x14, 14 * 8(sp)
    sd x15, 15 * 8(sp)
    sd x16, 16 * 8(sp)
    sd x17, 17 * 8(sp)
    sd x18, 18 * 8(sp)
    sd x19, 19 * 8(sp)
    sd x20, 20 * 8(sp)
    sd x21, 21 * 8(sp)
    sd x22, 22 * 8(sp)
    sd x23, 23 * 8(sp)
    sd x24, 24 * 8(sp)
    sd x25, 25 * 8(sp)
    sd x26, 26 * 8(sp)
    sd x27, 27 * 8(sp)
    sd x28, 28 * 8(sp)
    sd x29, 29 * 8(sp)
    sd x30, 30 * 8(sp)
    sd x31, 31 * 8(sp)
    //save return pc
    csrr t0, sepc
    sd t0, 0 * 8(sp)
    //save status
    csrr t0, sstatus
    sd t0, 32 * 8(sp) 
    //save sp
    csrr t0, sscratch
    sd t0 , 2* 8(sp)
	.endm

/*
 * Prepares to return from a system call or exception by restoring all
 * registers from the stack.
 */
	.macro RESTORE_ALL
    //restore return pc
    ld  t0,   0 * 8(sp)
    csrw   sepc, t0 
    //restore status
    ld  t0,   32 * 8(sp) 
    csrw   sstatus, t0

    //restor user stack pointer
    ld t0, 2 * 8(sp)
    csrw    sscratch, t0

    //restore generic register
    ld x1,   1 * 8(sp)
    ld x3,   3 * 8(sp)
    ld x4,   4 * 8(sp)
    ld x5,   5 * 8(sp)
    ld x6,   6 * 8(sp)
    ld x7,   7 * 8(sp)
    ld x8,   8 * 8(sp)
    ld x9,   9 * 8(sp)
    ld x10, 10 * 8(sp)
    ld x11, 11 * 8(sp)
    ld x12, 12 * 8(sp)
    ld x13, 13 * 8(sp)
    ld x14, 14 * 8(sp)
    ld x15, 15 * 8(sp)
    ld x16, 16 * 8(sp)
    ld x17, 17 * 8(sp)
    ld x18, 18 * 8(sp)
    ld x19, 19 * 8(sp)
    ld x20, 20 * 8(sp)
    ld x21, 21 * 8(sp)
    ld x22, 22 * 8(sp)
    ld x23, 23 * 8(sp)
    ld x24, 24 * 8(sp)
    ld x25, 25 * 8(sp)
    ld x26, 26 * 8(sp)
    ld x27, 27 * 8(sp)
    ld x28, 28 * 8(sp)
    ld x29, 29 * 8(sp)
    ld x30, 30 * 8(sp)
    ld x31, 31 * 8(sp)

    addi sp, sp, 36 * 8
    csrrw sp, sscratch, sp
	.endm

.global interrupt_table_start, interrupt_table_end
interrupt_table_start: /*interrupt table, with syscall and irq items*/
    j    trap_entry 
interrupt_table_end:

	/* trap entry */
	.align 2
	.global trap_entry
trap_entry:
    SAVE_ALL

	csrr a0, scause
	csrr a1, sepc
	csrr a2, stval
	mv a3, sp
    
	jal handle_trap

    RESTORE_ALL
	sret