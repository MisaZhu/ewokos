#ifndef EWOKOS_LIBC_ASSERT_H
#define EWOKOS_LIBC_ASSERT_H

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) ((void)sizeof(expr))
#endif

#endif
