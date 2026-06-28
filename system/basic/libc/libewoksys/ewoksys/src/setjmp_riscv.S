/*
 * setjmp/longjmp for RISC-V 64 (EwokOS)
 *
 * Saves/restores callee-saved registers: ra, sp, s0-s11
 * jmp_buf layout: 14 registers × 8 bytes = 112 bytes (fits in long[24] = 192 bytes)
 */

.text
.global setjmp
.type setjmp, @function
setjmp:
    sd ra,  0(a0)
    sd sp,  8(a0)
    sd s0,  16(a0)
    sd s1,  24(a0)
    sd s2,  32(a0)
    sd s3,  40(a0)
    sd s4,  48(a0)
    sd s5,  56(a0)
    sd s6,  64(a0)
    sd s7,  72(a0)
    sd s8,  80(a0)
    sd s9,  88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)
    li a0, 0
    ret
.size setjmp, .-setjmp

.global longjmp
.type longjmp, @function
longjmp:
    ld ra,  0(a0)
    ld sp,  8(a0)
    ld s0,  16(a0)
    ld s1,  24(a0)
    ld s2,  32(a0)
    ld s3,  40(a0)
    ld s4,  48(a0)
    ld s5,  56(a0)
    ld s6,  64(a0)
    ld s7,  72(a0)
    ld s8,  80(a0)
    ld s9,  88(a0)
    ld s10, 96(a0)
    ld s11, 104(a0)
    /* return value must be at least 1 */
    seqz a0, a1
    add a0, a0, a1
    ret
.size longjmp, .-longjmp

/* Custom-named entry points to avoid GCC __builtin_setjmp interception */

.global gumbo_setjmp
.type gumbo_setjmp, @function
gumbo_setjmp:
    sd ra,  0(a0)
    sd sp,  8(a0)
    sd s0,  16(a0)
    sd s1,  24(a0)
    sd s2,  32(a0)
    sd s3,  40(a0)
    sd s4,  48(a0)
    sd s5,  56(a0)
    sd s6,  64(a0)
    sd s7,  72(a0)
    sd s8,  80(a0)
    sd s9,  88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)
    li a0, 0
    ret
.size gumbo_setjmp, .-gumbo_setjmp

.global gumbo_longjmp
.type gumbo_longjmp, @function
gumbo_longjmp:
    ld ra,  0(a0)
    ld sp,  8(a0)
    ld s0,  16(a0)
    ld s1,  24(a0)
    ld s2,  32(a0)
    ld s3,  40(a0)
    ld s4,  48(a0)
    ld s5,  56(a0)
    ld s6,  64(a0)
    ld s7,  72(a0)
    ld s8,  80(a0)
    ld s9,  88(a0)
    ld s10, 96(a0)
    ld s11, 104(a0)
    seqz a0, a1
    add a0, a0, a1
    ret
.size gumbo_longjmp, .-gumbo_longjmp
