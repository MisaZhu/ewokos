/*
 * setjmp/longjmp for ARM32 (EwokOS)
 *
 * Saves/restores callee-saved registers: r4-r11, sp, lr
 * jmp_buf layout: 10 registers × 4 bytes = 40 bytes (fits in long[24] = 96 bytes)
 */

.text
.global setjmp
.type setjmp, %function
setjmp:
    stmia r0, {r4-r11, sp, lr}
    mov r0, #0
    bx lr
.size setjmp, .-setjmp

.global longjmp
.type longjmp, %function
longjmp:
    ldmia r0, {r4-r11, sp, lr}
    movs r0, r1
    moveq r0, #1
    bx lr
.size longjmp, .-longjmp

/* Custom-named entry points to avoid GCC __builtin_setjmp interception */

.global gumbo_setjmp
.type gumbo_setjmp, %function
gumbo_setjmp:
    stmia r0, {r4-r11, sp, lr}
    mov r0, #0
    bx lr
.size gumbo_setjmp, .-gumbo_setjmp

.global gumbo_longjmp
.type gumbo_longjmp, %function
gumbo_longjmp:
    ldmia r0, {r4-r11, sp, lr}
    movs r0, r1
    moveq r0, #1
    bx lr
.size gumbo_longjmp, .-gumbo_longjmp
