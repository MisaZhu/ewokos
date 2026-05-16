#ifndef EWOKOS_LIBC_SETJMP_H
#define EWOKOS_LIBC_SETJMP_H

typedef long jmp_buf[8];

#ifdef __cplusplus
extern "C" {
#endif

void ewok_longjmp(jmp_buf env, int val);

#ifdef __cplusplus
}
#endif

/*
 * `setjmp`/`longjmp` must expand at the call site so the compiler can
 * generate the target-specific control-flow save/restore sequence.
 * Wrapping GCC builtins in a normal C function breaks this on AArch64.
 */
#define setjmp(env) __builtin_setjmp(env)
#define longjmp ewok_longjmp

#endif
