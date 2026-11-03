#ifndef EWOKOS_REGEX_H
#define EWOKOS_REGEX_H

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * POSIX 1003.2 regular-expression interface.
 *
 * Both Basic (BRE) and Extended (ERE) regular expressions are supported,
 * selected with REG_EXTENDED.  The matcher is a backtracking engine, so it
 * reports the leftmost match and, from there, prefers earlier alternatives
 * and greedy repetition (leftmost-first).  This agrees with POSIX for the
 * overwhelmingly common patterns and is what GNU grep relies on; it can
 * differ from a strict leftmost-longest DFA on contrived alternations.
 *
 * A handful of GNU/grep extensions are accepted in both syntaxes because
 * they are widely used on the command line and never change the meaning of
 * a strictly-POSIX pattern:  \w \W \s \S \< \> \b \B and (?:...) groups.
 */

/* Signed byte offset, wide enough for any string index. */
typedef ssize_t regoff_t;

/* One captured subexpression: [rm_so, rm_eo). Both are -1 when unset. */
typedef struct {
	regoff_t rm_so;
	regoff_t rm_eo;
} regmatch_t;

/*
 * Compiled regular expression.  Only re_nsub is part of the POSIX contract
 * (the number of parenthesized subexpressions in the pattern).  The rest are
 * private handles so the struct stays small and the ABI stays stable; they
 * must not be touched by callers.
 */
typedef struct {
	size_t re_nsub;     /* number of capturing subexpressions */
	int    re_cflags;   /* cflags handed to regcomp() */
	int    re_err;      /* REG_* code from the last compile, 0 on success */
	void  *re_prog;     /* private: instruction array (re_insn[]) */
	size_t re_nprog;    /* private: instruction count */
	void  *re_classes;  /* private: bracket-expression table (re_class[]) */
	size_t re_nclass;   /* private: bracket-expression count */
} regex_t;

/* cflags for regcomp() */
#define REG_EXTENDED  0x01  /* use Extended Regular Expression syntax */
#define REG_ICASE     0x02  /* ignore case */
#define REG_NEWLINE   0x04  /* make ^ $ . and [^...] newline-sensitive */
#define REG_NOSUB     0x08  /* regexec() need not report match offsets */

/* eflags for regexec() */
#define REG_NOTBOL    0x01  /* the string does not start a line (^ won't match) */
#define REG_NOTEOL    0x02  /* the string does not end a line ($ won't match) */

/* regcomp()/regexec() return codes */
#define REG_NOERROR     0   /* success */
#define REG_NOMATCH     1   /* regexec() found no match */
#define REG_BADPAT      2   /* invalid regular expression */
#define REG_ECOLLATE    3   /* invalid collating element reference */
#define REG_ECTYPE      4   /* invalid character class reference */
#define REG_EESCAPE     5   /* trailing backslash (\) */
#define REG_ESUBREG     6   /* invalid back reference */
#define REG_EBRACK      7   /* brackets ([ ]) not balanced */
#define REG_EPAREN      8   /* parentheses (( )) not balanced */
#define REG_EBRACE      9   /* braces ({ }) not balanced */
#define REG_BADBR      10   /* invalid repeat count(s) in { } */
#define REG_ERANGE     11   /* invalid character range in [ ] */
#define REG_ESPACE     12   /* out of memory */
#define REG_BADRPT     13   /* repetition operator in an invalid position */

int    regcomp(regex_t *preg, const char *pattern, int cflags);
int    regexec(const regex_t *preg, const char *string, size_t nmatch,
               regmatch_t pmatch[], int eflags);
size_t regerror(int errcode, const regex_t *preg, char *errbuf,
                size_t errbuf_size);
void   regfree(regex_t *preg);

#ifdef __cplusplus
}
#endif

#endif
