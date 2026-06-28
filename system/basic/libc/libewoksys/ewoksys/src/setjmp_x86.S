/*
 * setjmp/longjmp for x86-64 (EwokOS)
 *
 * Saves/restores callee-saved registers: rbx, rbp, r12-r15, rsp, rip(return addr)
 * jmp_buf layout: 8 registers × 8 bytes = 64 bytes (fits in long[24] = 192 bytes)
 */

.text
.global setjmp
.type setjmp, @function
setjmp:
    mov %rbx, (%rdi)
    mov %rbp, 8(%rdi)
    mov %r12, 16(%rdi)
    mov %r13, 24(%rdi)
    mov %r14, 32(%rdi)
    mov %r15, 40(%rdi)
    lea 8(%rsp), %rax       /* save caller's rsp (before call pushed return addr) */
    mov %rax, 48(%rdi)
    mov (%rsp), %rax        /* save return address */
    mov %rax, 56(%rdi)
    xor %eax, %eax
    ret
.size setjmp, .-setjmp

.global longjmp
.type longjmp, @function
longjmp:
    mov (%rdi), %rbx
    mov 8(%rdi), %rbp
    mov 16(%rdi), %r12
    mov 24(%rdi), %r13
    mov 32(%rdi), %r14
    mov 40(%rdi), %r15
    mov 48(%rdi), %rsp
    mov %esi, %eax          /* return value */
    test %eax, %eax
    jnz 1f
    inc %eax                /* must return at least 1 */
1:
    jmp *56(%rdi)           /* jump to saved return address */
.size longjmp, .-longjmp

/* Custom-named entry points to avoid GCC __builtin_setjmp interception */

.global gumbo_setjmp
.type gumbo_setjmp, @function
gumbo_setjmp:
    mov %rbx, (%rdi)
    mov %rbp, 8(%rdi)
    mov %r12, 16(%rdi)
    mov %r13, 24(%rdi)
    mov %r14, 32(%rdi)
    mov %r15, 40(%rdi)
    lea 8(%rsp), %rax
    mov %rax, 48(%rdi)
    mov (%rsp), %rax
    mov %rax, 56(%rdi)
    xor %eax, %eax
    ret
.size gumbo_setjmp, .-gumbo_setjmp

.global gumbo_longjmp
.type gumbo_longjmp, @function
gumbo_longjmp:
    mov (%rdi), %rbx
    mov 8(%rdi), %rbp
    mov 16(%rdi), %r12
    mov 24(%rdi), %r13
    mov 32(%rdi), %r14
    mov 40(%rdi), %r15
    mov 48(%rdi), %rsp
    mov %esi, %eax
    test %eax, %eax
    jnz 2f
    inc %eax
2:
    jmp *56(%rdi)
.size gumbo_longjmp, .-gumbo_longjmp
