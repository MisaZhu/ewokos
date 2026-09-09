/*
 * setlocale() and localeconv() for a C-locale-only system.
 *
 * See include/locale.h for why the only honest answers are "C" and NULL, and
 * why a query can never fail.
 *
 * There is no state to keep.  Every category is "C" from process start and
 * nothing here can move it, so the "current locale name" a query returns is a
 * constant.  That is what makes this whole file fit in a handful of lines: a
 * real setlocale() has to parse names like de_DE.UTF-8@euro, load and validate
 * archive files, keep per-category pointers and roll back on partial failure.
 * None of that exists because none of it can succeed.
 */

#include <locale.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

/*
 * The C locale's formatting conventions, exactly as C99 7.11.2.1 spells them
 * out.  Two details are easy to get wrong and matter:
 *
 *   thousands_sep and mon_thousands_sep are "", not NULL.  A NULL there would
 *   send every printf("%'d") implementation and every strtod that walks the
 *   grouping into a null dereference.
 *
 *   grouping and mon_grouping are "", meaning "no grouping at all".  The other
 *   way to say that is "\177", which means the same thing but only for
 *   implementations that read it; "" is the form the standard names first and
 *   the one musl and glibc's C locale both use.
 *
 * Every char member is CHAR_MAX, which C99 defines as "not available in this
 * locale" - it is the standard's own value for all fourteen of them in the C
 * locale, not a placeholder.  Filling them with zeros instead would be read by
 * a locale-aware formatter as "zero decimal places, currency symbol preceded by
 * nothing, sign at position 0", which is a confident wrong answer rather than
 * an admitted absence.
 */
static char ewok_decimal_point[] = ".";
static char ewok_empty[] = "";

static struct lconv ewok_lconv = {
	/* decimal_point     */ ewok_decimal_point,
	/* thousands_sep     */ ewok_empty,
	/* grouping          */ ewok_empty,
	/* int_curr_symbol   */ ewok_empty,
	/* currency_symbol   */ ewok_empty,
	/* mon_decimal_point */ ewok_empty,
	/* mon_thousands_sep */ ewok_empty,
	/* mon_grouping      */ ewok_empty,
	/* positive_sign     */ ewok_empty,
	/* negative_sign     */ ewok_empty,
	/* int_frac_digits   */ CHAR_MAX,
	/* frac_digits       */ CHAR_MAX,
	/* p_cs_precedes     */ CHAR_MAX,
	/* p_sep_by_space    */ CHAR_MAX,
	/* n_cs_precedes     */ CHAR_MAX,
	/* n_sep_by_space    */ CHAR_MAX,
	/* p_sign_posn       */ CHAR_MAX,
	/* n_sign_posn       */ CHAR_MAX,
	/* int_p_cs_precedes */ CHAR_MAX,
	/* int_n_cs_precedes */ CHAR_MAX,
	/* int_p_sep_by_space */ CHAR_MAX,
	/* int_n_sep_by_space */ CHAR_MAX,
	/* int_p_sign_posn   */ CHAR_MAX,
	/* int_n_sign_posn   */ CHAR_MAX
};

/*
 * The static string POSIX says setlocale() returns.  It is not const so that
 * the prototype can be `char *`, which is what every caller expects - qstrdup()
 * and QByteArray(const char *) both take a non-const char * in the C API.
 * Callers are not allowed to write through it, and none do.
 */
static char ewok_locale_name[] = EWOK_LOCALE_NAME;

char *setlocale(int category, const char *locale) {
	if (category < LC_CTYPE || category > LC_ALL) {
		errno = EINVAL;
		return NULL;
	}

	/* A query.  Cannot fail: the current name is a constant. */
	if (locale == NULL)
		return ewok_locale_name;

	/* The empty string asks for the implementation-defined default locale,
	   which - with no locale archive to consult and nothing in the
	   environment that could name one - is "C".  Honoring $LANG here would
	   mean reporting a locale this libc cannot implement, and then every
	   formatting function would disagree with the name it just handed out. */
	if (locale[0] == '\0')
		return ewok_locale_name;

	if (strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0)
		return ewok_locale_name;

	/* An unsupported name.  errno is left alone: POSIX does not require
	   setlocale() to set it, and inventing a value here would be a guess
	   that a caller might branch on.  The NULL return is the signal. */
	return NULL;
}

struct lconv *localeconv(void) {
	return &ewok_lconv;
}
