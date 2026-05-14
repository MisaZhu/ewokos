#ifndef EWOKOS_LIBC_ASSERT_H
#define EWOKOS_LIBC_ASSERT_H

#include <stdlib.h>

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) do { if (!(expr)) abort(); } while (0)
#endif

#endif
