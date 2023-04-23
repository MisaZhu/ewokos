/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013 Regents of the University of California
 */

#include <asm.h>
#include <linkage.h>

/* void *memcpy(void *, const void *, size_t) */
.global __memcpy32
__memcpy32:
	/* Save for return value */
	mv	t6, a0

	/*
	 * Register allocation for code below:
	 * a0 - start of uncopied dst
	 * a1 - start of uncopied src
	 * t0 - end of uncopied dst
	 */
	add	t0, a0, a2

	/*
	 * Use bytewise copy if too small.
	 *
	 * This threshold must be at least 2*8 to ensure at least one
	 * wordwise copy is performed. It is chosen to be 16 because it will
	 * save at least 7 iterations of bytewise copy, which pays off the
	 * fixed overhead.
	 */
	li	a3, 16
	bltu	a2, a3, .Lbyte_copy_tail

	/*
	 * Bytewise copy first to align a0 to word boundary.
	 */
	addi	a2, a0, 8-1
	andi	a2, a2, ~(8-1)
	beq	a0, a2, 2f
1:
	lb	a5, 0(a1)
	addi	a1, a1, 1
	sb	a5, 0(a0)
	addi	a0, a0, 1
	bne	a0, a2, 1b
2:

	/*
	 * Now a0 is word-aligned. If a1 is also word aligned, we could perform
	 * aligned word-wise copy. Otherwise we need to perform misaligned
	 * word-wise copy.
	 */
	andi	a3, a1, 8-1
	bnez	a3, .Lmisaligned_word_copy

	/* Unrolled wordwise copy */
	addi	t0, t0, -(16*8-1)
	bgeu	a0, t0, 2f
1:
	ld	a2,        0(a1)
	ld	a3,    8(a1)
	ld	a4,  2*8(a1)
	ld	a5,  3*8(a1)
	ld	a6,  4*8(a1)
	ld	a7,  5*8(a1)
	ld	t1,  6*8(a1)
	ld	t2,  7*8(a1)
	ld	t3,  8*8(a1)
	ld	t4,  9*8(a1)
	ld	t5, 10*8(a1)
	sd	a2,    0(a0)
	sd	a3,    8(a0)
	sd	a4,  2*8(a0)
	sd	a5,  3*8(a0)
	sd	a6,  4*8(a0)
	sd	a7,  5*8(a0)
	sd	t1,  6*8(a0)
	sd	t2,  7*8(a0)
	sd	t3,  8*8(a0)
	sd	t4,  9*8(a0)
	sd	t5, 10*8(a0)
	ld	a2, 11*8(a1)
	ld	a3, 12*8(a1)
	ld	a4, 13*8(a1)
	ld	a5, 14*8(a1)
	ld	a6, 15*8(a1)
	addi	a1, a1, 16*8
	sd	a2, 11*8(a0)
	sd	a3, 12*8(a0)
	sd	a4, 13*8(a0)
	sd	a5, 14*8(a0)
	sd	a6, 15*8(a0)
	addi	a0, a0, 16*8
	bltu	a0, t0, 1b
2:
	/* Post-loop increment by 16*8-1 and pre-loop decrement by 8-1 */
	addi	t0, t0, 15*8

	/* Wordwise copy */
	bgeu	a0, t0, 2f
1:
	ld	a5, 0(a1)
	addi	a1, a1, 8
	sd	a5, 0(a0)
	addi	a0, a0, 8
	bltu	a0, t0, 1b
2:
	addi	t0, t0, 8-1

.Lbyte_copy_tail:
	/*
	 * Bytewise copy anything left.
	 */
	beq	a0, t0, 2f
1:
	lb	a5, 0(a1)
	addi	a1, a1, 1
	sb	a5, 0(a0)
	addi	a0, a0, 1
	bne	a0, t0, 1b
2:

	mv	a0, t6
	ret

.Lmisaligned_word_copy:
	/*
	 * Misaligned word-wise copy.
	 * For misaligned copy we still perform word-wise copy, but we need to
	 * use the value fetched from the previous iteration and do some shifts.
	 * This is safe because we wouldn't access more words than necessary.
	 */

	/* Calculate shifts */
	slli	t3, a3, 3
	sub	t4, x0, t3 /* negate is okay as shift will only look at LSBs */

	/* Load the initial value and align a1 */
	andi	a1, a1, ~(8-1)
	ld	a5, 0(a1)

	addi	t0, t0, -(8-1)
	/* At least one iteration will be executed here, no check */
1:
	srl	a4, a5, t3
	ld	a5, 8(a1)
	addi	a1, a1, 8
	sll	a2, a5, t4
	or	a2, a2, a4
	sd	a2, 0(a0)
	addi	a0, a0, 8
	bltu	a0, t0, 1b

	/* Update pointers to correct value */
	addi	t0, t0, 8-1
	add	a1, a1, a3

	j	.Lbyte_copy_tail
