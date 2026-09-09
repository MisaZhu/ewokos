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

/*
 * wcslen - the one wide-character function that is not a conversion.
 *
 * The comment at the top of this file says no conversion functions are
 * declared, and that refusal is load-bearing: libcurses is built with
 * DISABLE_WCHAR against this header, and pulling in newlib's wcstombs/mbstowcs
 * family would drag in a stdio/mbstate_t ABI EwokOS does not have.
 *
 * wcslen is different in kind.  It converts nothing - it counts wchar_t
 * elements until a zero, needing no locale, no mbstate_t and no stdio.  It is
 * therefore declared here and defined in src/string/wcslen.c, alongside the
 * other byte-oriented string helpers.
 *
 * Two consumers, which is why it lands in the C header rather than only in the
 * C++ <cwchar>:
 *   qstring.cpp   QString::fromWCharArray / toStdWString paths call the
 *                 *global* wcslen, where a namespace-std-only declaration is
 *                 not visible.
 *   <cwchar>      EWOK_STL's std::wcslen/wcsxfrm keep working unchanged;
 *                 declaring ::wcslen does not conflict with it, because
 *                 inside namespace std the std:: declaration is found first.
 */
#ifdef __cplusplus
extern "C" {
#endif

size_t wcslen(const wchar_t *s);

/*
 * wcwidth clears the same bar wcslen does: a pure table lookup over the
 * Unicode non-spacing and East Asian Wide ranges, no locale, no mbstate_t,
 * no stdio.  Defined in src/string/wcwidth.c; the consumer that forced it is
 * qtermwidget's terminal emulation, which asks for every cell whether a
 * glyph spans one column or two.
 */
int wcwidth(wchar_t wc);

#ifdef __cplusplus
}
#endif

#endif
