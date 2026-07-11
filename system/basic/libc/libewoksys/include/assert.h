#ifndef EWOKOS_LIBC_ASSERT_H
#define EWOKOS_LIBC_ASSERT_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void __assert_func(const char *file, int line, const char *func, const char *expr);
void __assert_fail(const char *expr, const char *file, int line, const char *func);

#ifdef __cplusplus
}
#endif

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) \
	do { \
		if (!(expr)) \
			__assert_func(__FILE__, __LINE__, __func__, #expr); \
	} while (0)
#endif

#endif
