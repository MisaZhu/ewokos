#ifndef EWOKOS_LIBC__ANSI_H
#define EWOKOS_LIBC__ANSI_H

#ifdef __cplusplus
#define _BEGIN_STD_C extern "C" {
#define _END_STD_C }
#else
#define _BEGIN_STD_C
#define _END_STD_C
#endif

#define _PTR void *
#define _CONST const
#define _EXFUN(name, proto) name proto
#define _DEFUN(name, arglist, args) name args
#define _AND ,
#define _NOARGS void

#endif
