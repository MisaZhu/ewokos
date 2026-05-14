#ifndef EWOKOS_LIBC_SETJMP_H
#define EWOKOS_LIBC_SETJMP_H

typedef long jmp_buf[8];

#ifdef __cplusplus
extern "C" {
#endif

int ewok_setjmp(jmp_buf env);
void ewok_longjmp(jmp_buf env, int val);

#ifdef __cplusplus
}
#endif

#define setjmp ewok_setjmp
#define longjmp ewok_longjmp

#endif
