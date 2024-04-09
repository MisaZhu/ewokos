#ifndef SVC_CALL_H
#define SVC_CALL_H

#include <ewoksys/ewokdef.h>
#include <syscalls.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int syscall3_raw(int, int, int, int);

#define syscall3(code, arg0, arg1, arg2) syscall3_raw((code), (arg0), (arg1), (arg2))
#define syscall2(code, arg0, arg1) syscall3_raw((code), (arg0), (arg1), 0)
#define syscall1(code, arg0) syscall3_raw((code), (arg0), 0, 0)
#define syscall0(code) syscall3_raw((code), 0, 0, 0)

#ifdef __cplusplus
}
#endif

#endif
