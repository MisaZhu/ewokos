.section ".init"

.global __entry
__entry:
    /* Disable interrupts */
    csrw    sie, zero
    csrw    sip, zero
	
	/* Load the global pointer */
.option push
.option norelax
	la gp,  _svc_start_stack
.option pop

    /*
     * Disable FPU to detect illegal usage of
     * floating point in kernel space
     */
    li t0, 0x00006000
    csrc sstatus, t0

	/* Pick one hart to run the main boot sequence */
    #lla a3, _hart_lottery
    #li  a2, 1
    #amoadd.w a3, a2, (a3)
    #bnez    a3, _panic

    /* Clear BSS for flat non-ELF images */
    la a3, _bss_start
    la a4, _bss_end
    ble a4, a3, clear_bss_done
clear_bss:
    sd zero, (a3)
    add a3, a3, 8
    blt a3, a4, clear_bss
clear_bss_done:

    /* Setup stack */
    lla a3, _svc_start_stack
	mv  sp, a3
    lla  t0, _irq_stack
    csrw  sscratch, t0

    /* Setup global pointer*/
    mv  gp, zero

    /*enable user memory access permission*/
    li   t0, 0x40000
    csrs sstatus,t0 

	call   _boot_start

    lla  sp, _svc_stack
    call _kernel_entry_c

.global _panic
_panic:
	wfi
	j _panic