/*
 * setjmp/longjmp for AArch64 (EwokOS)
 *
 * Saves/restores all callee-saved registers:
 *   x19-x28, x29 (FP), x30 (LR), sp, d8-d15
 *
 * jmp_buf layout (offsets in bytes):
 *   [0]   x19, x20
 *   [16]  x21, x22
 *   [32]  x23, x24
 *   [48]  x25, x26
 *   [64]  x27, x28
 *   [80]  x29, x30
 *   [96]  sp
 *   [104] d8, d9
 *   [120] d10, d11
 *   [136] d12, d13
 *   [152] d14, d15
 */

.text
.global setjmp
.type setjmp, %function
setjmp:
    /* x0 = pointer to jmp_buf */
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    stp x29, x30, [x0, #80]
    mov x2, sp
    str x2, [x0, #96]
    stp d8, d9, [x0, #104]
    stp d10, d11, [x0, #120]
    stp d12, d13, [x0, #136]
    stp d14, d15, [x0, #152]
    mov x0, #0
    ret
.size setjmp, .-setjmp

.global longjmp
.type longjmp, %function
longjmp:
    /* x0 = pointer to jmp_buf, x1 = return value */
    ldp x19, x20, [x0, #0]
    ldp x21, x22, [x0, #16]
    ldp x23, x24, [x0, #32]
    ldp x25, x26, [x0, #48]
    ldp x27, x28, [x0, #64]
    ldp x29, x30, [x0, #80]
    ldr x2, [x0, #96]
    mov sp, x2
    ldp d8, d9, [x0, #104]
    ldp d10, d11, [x0, #120]
    ldp d12, d13, [x0, #136]
    ldp d14, d15, [x0, #152]
    /* return value must be at least 1 */
    cmp x1, #0
    csinc x0, x1, xzr, ne
    ret
.size longjmp, .-longjmp

/* Custom-named entry points to avoid GCC __builtin_setjmp interception */

.global gumbo_setjmp
.type gumbo_setjmp, %function
gumbo_setjmp:
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    stp x29, x30, [x0, #80]
    mov x2, sp
    str x2, [x0, #96]
    stp d8, d9, [x0, #104]
    stp d10, d11, [x0, #120]
    stp d12, d13, [x0, #136]
    stp d14, d15, [x0, #152]
    mov x0, #0
    ret
.size gumbo_setjmp, .-gumbo_setjmp

.global gumbo_longjmp
.type gumbo_longjmp, %function
gumbo_longjmp:
    ldp x19, x20, [x0, #0]
    ldp x21, x22, [x0, #16]
    ldp x23, x24, [x0, #32]
    ldp x25, x26, [x0, #48]
    ldp x27, x28, [x0, #64]
    ldp x29, x30, [x0, #80]
    ldr x2, [x0, #96]
    mov sp, x2
    ldp d8, d9, [x0, #104]
    ldp d10, d11, [x0, #120]
    ldp d12, d13, [x0, #136]
    ldp d14, d15, [x0, #152]
    cmp x1, #0
    csinc x0, x1, xzr, ne
    ret
.size gumbo_longjmp, .-gumbo_longjmp
