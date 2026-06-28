#ifndef EWOKOS_LIBC_SETJMP_H
#define EWOKOS_LIBC_SETJMP_H

/* 24 × 8 = 192 bytes: enough for all aarch64 callee-saved registers
 * Layout (offsets in bytes):
 *   [0]   x19-x20
 *   [16]  x21-x22
 *   [32]  x23-x24
 *   [48]  x25-x26
 *   [64]  x27-x28
 *   [80]  x29 (FP), x30 (LR)
 *   [96]  sp
 *   [104] d8-d9
 *   [120] d10-d11
 *   [136] d12-d13
 *   [152] d14-d15
 */
typedef long jmp_buf[24];

#ifdef __cplusplus
extern "C" {
#endif

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val) __attribute__((noreturn));
void ewok_longjmp(jmp_buf env, int val);

#ifdef __cplusplus
}
#endif

#endif
