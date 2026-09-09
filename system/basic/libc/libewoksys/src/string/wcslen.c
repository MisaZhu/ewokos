#include <wchar.h>

/*
 * Length of a wide-character string, excluding the terminator.
 *
 * This is the only wcs* function EwokOS provides, and the reason it can be
 * provided at all is that it performs no conversion: it counts wchar_t
 * elements until it hits a zero, needing no locale, no mbstate_t and no stdio.
 * Everything else in the wide-character API (wcstombs, mbstowcs, the printf
 * family's %ls) does need those, which is why <wchar.h> refuses to declare
 * them - see the comment at the top of that header.
 *
 * It lives here in src/string/ rather than src/wchar/ because it is a string
 * helper and there is no other wide-character source directory to justify.
 */
size_t wcslen(const wchar_t *s) {
    const wchar_t *p = s;

    while (*p != (wchar_t)0) {
        ++p;
    }

    return (size_t)(p - s);
}
