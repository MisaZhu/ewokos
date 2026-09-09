#ifndef EWOKOS_LIBC_LOCALE_H
#define EWOKOS_LIBC_LOCALE_H

/*
 * <locale.h> for a C-locale-only system.
 *
 * EwokOS ships no locale tables: there is no /usr/lib/locale, no message
 * catalogues, no per-locale collation weights (src/string/strcoll.c already
 * documents this and falls back to strcmp), and no alternate decimal points.
 * So every category is permanently "C", and setlocale() can only ever report
 * that or refuse.
 *
 * Refusing is not optional.  Callers do two different things with the result:
 *
 *   setlocale(cat, "")      - install the environment's locale, and Qt does
 *                             exactly this in QCoreApplicationPrivate::initLocale().
 *                             Answering "C" is correct: the environment has no
 *                             locales to offer, so "C" is the locale it selects.
 *
 *   setlocale(cat, NULL)    - *query* the current locale name.  QTextCodec::
 *                             codecForLocale() feeds the result straight into
 *                             QByteArray(const char *), which calls qstrlen().
 *                             Returning NULL for a query would be a null
 *                             dereference in the caller, so a query always
 *                             succeeds here.
 *
 * The one thing this header must not do is quietly accept a locale name it
 * cannot honour.  setlocale(LC_ALL, "de_DE.UTF-8") returning "C" would let a
 * program believe it has German collation and a comma decimal point while the
 * library underneath it still formats with '.' - the classic locale bug, and
 * one that is invisible at the call site.  An unsupported name returns NULL,
 * which is what POSIX asks for and what a caller can check.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Category values.  These are plain ints and their numbering is part of this
 * libc's ABI: the six real categories run 0..5 in POSIX's order and LC_ALL,
 * which means "all of them", sits after them at 6 - so the validity check in
 * locale.c is the single range test `category < LC_CTYPE || category > LC_ALL`.
 */
#define LC_CTYPE    0
#define LC_NUMERIC  1
#define LC_TIME     2
#define LC_COLLATE  3
#define LC_MONETARY 4
#define LC_MESSAGES 5
#define LC_ALL      6

/* The name every successful setlocale() call reports.  POSIX requires the
   returned string to be usable as the locale argument of a later call, and
   "C" is.  It is also the string QTextCodec::codecForLocale() special-cases
   when it decides what codec the locale implies. */
#define EWOK_LOCALE_NAME "C"

struct lconv {
	char *decimal_point;
	char *thousands_sep;
	char *grouping;
	char *int_curr_symbol;
	char *currency_symbol;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
	char int_p_cs_precedes;
	char int_n_cs_precedes;
	char int_p_sep_by_space;
	char int_n_sep_by_space;
	char int_p_sign_posn;
	char int_n_sign_posn;
};

/*
 * Sets or queries a locale category.  `locale` may be:
 *   NULL          - query; returns the current name, never fails.
 *   ""            - the implementation-defined default, which here is "C".
 *   "C" / "POSIX" - the C locale, the only one installed.
 *   anything else - unsupported; returns NULL and changes nothing.
 *
 * Returns a pointer to a static string, or NULL on failure.  The string stays
 * valid until the next setlocale() call for the same category, which is the
 * POSIX lifetime and the reason it is safe to hand it to qstrdup().
 */
char *setlocale(int category, const char *locale);

/* Numeric and monetary formatting for the current locale - always the C one.
   Returns a pointer to a static object; never NULL. */
struct lconv *localeconv(void);

#ifdef __cplusplus
}
#endif

#endif /* EWOKOS_LIBC_LOCALE_H */
