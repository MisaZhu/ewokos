#ifndef EWOKOS_WCHAR_H
#define EWOKOS_WCHAR_H

/*
 * Minimal wide-character header.
 *
 * EwokOS has no wide/multibyte conversion runtime, so this header only
 * supplies the types and constants that portable code (e.g. libcurses built
 * with DISABLE_WCHAR) needs to parse: wchar_t (via <stddef.h>), wint_t and
 * the WCHAR limits. It intentionally shadows the toolchain's newlib wchar.h,
 * which cannot be compiled against EwokOS's stdio.h. No conversion functions
 * are provided or declared.
 */

#include <stddef.h>   /* wchar_t, size_t, NULL */

#ifndef __cplusplus
/* wchar_t is a builtin type in C++; in C it comes from <stddef.h>. */
#endif

#ifdef __WINT_TYPE__
typedef __WINT_TYPE__ wint_t;
#else
typedef unsigned int wint_t;
#endif

#ifndef WEOF
#define WEOF ((wint_t)-1)
#endif

#ifdef __WCHAR_MAX__
#ifndef WCHAR_MAX
#define WCHAR_MAX __WCHAR_MAX__
#endif
#else
#ifndef WCHAR_MAX
#define WCHAR_MAX 0x7fffffff
#endif
#endif

#ifndef WCHAR_MIN
#define WCHAR_MIN 0
#endif

#endif
