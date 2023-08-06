/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 */

/*
 * signed ret_idivmod_values(signed quot, signed rem);
 * return quotient and remaining the EABI way (regs r0,r1)
 */
.global ret_idivmod_values
ret_idivmod_values:
        bx lr

/*
 * unsigned ret_uidivmod_values(unsigned quot, unsigned rem);
 * return quotient and remaining the EABI way (regs r0,r1)
 */
.global ret_uidivmod_values
ret_uidivmod_values:
        bx      lr
