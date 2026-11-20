#ifndef NETBSD_SYS_CDEFS_H
#define NETBSD_SYS_CDEFS_H

#ifndef __RCSID
#define __RCSID(X)
#endif
#ifndef __BEGIN_DECLS
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#ifndef __COPYRIGHT
#define __COPYRIGHT(X)
#endif

#ifndef __printflike
#if __GNUC__ >= 3
#define __printflike(fmtarg, firstvararg)       \
            __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#define __scanflike(fmtarg, firstvararg)        \
            __attribute__((__format__ (__scanf__, fmtarg, firstvararg)))
#else
#define __printflike(fmtarg, firstvararg)       /* nothing */
#define __scanflike(fmtarg, firstvararg)        /* nothing */
#endif
#endif

#ifndef __warn_references
#define __warn_references(a, b)
#endif
#ifndef __P
#define __P(X) X
#endif
#ifndef __dead
#define __dead __attribute__((__noreturn__))
#endif
#define __UNCONST(a)   (void *)(intptr_t)(a)
# define __STRING(x)    #x
#if 0
#define __strong_alias(alias,sym)                                       \
        __asm__(".global " __STRING(alias) " ; " __STRING(alias)        \
            " = " __STRING(sym)) ;
#else
#undef myweak_alias
#define myweak_alias(old, new) \
        extern __typeof(old) new __attribute__((weak, alias(#old)))
#define __strong_alias(new, old) myweak_alias(old, new);
#endif
#ifndef _DIAGASSERT
#define _DIAGASSERT(x)
#endif
#ifndef __arraycount
#define __arraycount(a) (sizeof(a) / sizeof(*(a)))
#endif

#if __GNUC__ >= 4
#  define __dso_public  __attribute__((__visibility__("default")))
#  define __dso_hidden  __attribute__((__visibility__("hidden")))
#else
#  define __dso_public
#  define __dso_hidden
#endif
#if defined(__predict_true)
#elif __GNUC__ >= 3
#define __predict_true(exp)     __builtin_expect((exp) != 0, 1)
#define __predict_false(exp)    __builtin_expect((exp) != 0, 0)
#else
#define __predict_true(exp)     (exp)
#define __predict_false(exp)    (exp)
#endif

#endif

