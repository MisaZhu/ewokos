/*
 * signed ret_idivmod_values(signed quot, signed rem);
 * return quotient and remaining the EABI way (regs r0,r1)
 */
.global __ret_idivmod_values
__ret_idivmod_values:
    bx lr

/*
 * unsigned ret_uidivmod_values(unsigned quot, unsigned rem);
 * return quotient and remaining the EABI way (regs r0,r1)
 */
.global __ret_uidivmod_values
__ret_uidivmod_values:
    bx lr

