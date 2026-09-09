#include <string.h>

/*
 * Locale-aware collation.
 *
 * C99 7.11.4.4 specifies that strcoll returns a value with the same sign as
 * strcmp would when the current locale is the "C" locale, and EwokOS is
 * C-locale-only: there is no setlocale machinery, no LC_COLLATE tables, and
 * <locale.h> offers nothing to consult.  Delegating to strcmp is therefore
 * the correct implementation rather than a shortcut.
 *
 * If collation data is ever added, this is the single file that has to change;
 * callers reach it through the declaration in <string.h>.
 */
int strcoll(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}
