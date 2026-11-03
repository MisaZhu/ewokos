/*
 * grep - print lines that match a pattern.
 *
 * GNU grep compatible subset:
 *   grep [OPTION...] [-e] PATTERN [FILE...]
 *   grep [OPTION...] -e PATTERN... [-f FILE]... [FILE...]
 *
 * Pattern selection : -e --regexp, -f --file, -i --ignore-case,
 *                     -v --invert-match, -w --word-regexp, -x --line-regexp,
 *                     -E --extended-regexp, -F --fixed-strings,
 *                     -G --basic-regexp (default), -P --perl-regexp (=-E)
 * Matching control  : -m --max-count, --line-buffered, -z --null-data
 * General output    : -o --only-matching, -q --quiet/--silent, -s --no-messages,
 *                     -c --count, -L --files-without-match,
 *                     -l --files-with-matches, --color/--colour[=WHEN],
 *                     --group-separator, --no-group-separator
 * Context           : -A --after-context, -B --before-context, -C --context
 * Line prefix       : -b --byte-offset, -H --with-filename,
 *                     -h --without-filename, -n --line-number,
 *                     -T --initial-tab, -Z --null, --label, -u --unix-byte-offsets
 * File/dir selection: -a --text, -I --ignore-binary, --binary-files=ACTION,
 *                     -r --recursive, -R --dereference-recursive,
 *                     -d --directories, -D --devices, --include, --exclude,
 *                     --exclude-from, --exclude-dir
 * Misc              : --help, -V --version
 *
 * Exit status: 0 if any line was selected, 1 if none, 2 on error.
 *
 * Platform notes (EwokOS):
 *  - Matching uses the in-tree POSIX engine (<regex.h> in libewoksys): a
 *    leftmost-first backtracking matcher. Whether a line matches is identical
 *    to POSIX; only -o/--color offsets on rare overlapping alternations may
 *    differ from GNU's leftmost-longest DFA.
 *  - lstat()==stat(), so -r and -R (symlink following) behave identically.
 *  - --color defaults to "auto" (ANSI SGR, only when stdout is a tty). The
 *    GREP_COLORS environment variable is not parsed; GNU default colors are used.
 *  - Options are accepted both before and after the pattern/FILE operands.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <regex.h>
#include <fnmatch.h>
#include <ctype.h>
#include <sys/stat.h>

#define PROG            "grep"
#define STDIN_NAME      "(standard input)"
#define READ_CHUNK      65536

/* ------------------------------------------------------------------ */
/* option state                                                        */
/* ------------------------------------------------------------------ */
static int o_icase, o_invert, o_word, o_line;         /* -i -v -w -x */
static int o_extended, o_fixed;                       /* -E -F */
static int o_count, o_files_wo, o_files_w;            /* -c -L -l */
static int o_quiet, o_no_msgs, o_only_matching;       /* -q -s -o */
static int o_lineno, o_byteoff;                       /* -n -b */
static int o_with_fn, o_without_fn;                   /* -H -h */
static int o_init_tab, o_null_fn;                     /* -T -Z */
static int o_after, o_before;                         /* -A -B (in lines) */
static int o_recursive, o_deref_rec;                  /* -r -R */
static int o_text, o_ignore_bin;                      /* -a -I */
static int o_line_buf, o_null_data;                   /* --line-buffered -z */
static int o_color = 2;                               /* 0 never 1 always 2 auto */
static int o_use_color;                               /* resolved */
static long long o_max_count = -1;                    /* -m */
static int o_dir_action;   /* 0 read(default) 1 skip 2 recurse */
static int o_dev_action;   /* 0 read(default) 1 skip */
static char *o_label;
static char *o_group_sep = "--";
static int o_show_group_sep = 1;

static int g_exit = 1;             /* 1 none matched, 0 matched, 2 error */
static int g_show_fn;              /* resolved filename-prefix flag */
static long long g_matched_lines;  /* total selected lines (for -m across files) */

/* patterns */
typedef struct {
    char   *text;
    regex_t re;
    int     is_regex;
    size_t  len;
} pat_t;
static pat_t *g_pat;
static int    g_npat, g_cpat;

/* include / exclude glob lists */
static char **g_inc, **g_exc, **g_excdir;
static int g_ninc, g_nexc, g_nexcdir;

static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) { fprintf(stderr, "%s: out of memory\n", PROG); exit(2); }
    return p;
}
static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) { fprintf(stderr, "%s: out of memory\n", PROG); exit(2); }
    return q;
}
static char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = xmalloc(n);
    memcpy(p, s, n);
    return p;
}
static void list_add(char ***arr, int *n, int *cap_unused, const char *v) {
    (void)cap_unused;
    *arr = xrealloc(*arr, sizeof(char *) * (size_t)(*n + 1));
    (*arr)[(*n)++] = xstrdup(v);
}

/* ------------------------------------------------------------------ */
/* color helpers (GNU grep default palette)                            */
/* ------------------------------------------------------------------ */
#define C_SEL  "\033[01;31m"
#define C_CTX  "\033[31m"
#define C_FN   "\033[35m"
#define C_LN   "\033[32m"
#define C_SEP  "\033[36m"
#define C_OFF  "\033[m"

static void col(const char *code) { if (o_use_color) fputs(code, stdout); }

/* ------------------------------------------------------------------ */
/* small utilities                                                     */
/* ------------------------------------------------------------------ */
static int is_wordc(unsigned char c) { return isalnum(c) || c == '_'; }

static int str_eq(const char *a, const char *b, size_t n, int icase) {
    if (icase) return strncasecmp(a, b, n) == 0;
    return memcmp(a, b, n) == 0;
}

/* case-sensitive/insensitive substring search over a bounded buffer */
static int find_fixed(const char *hay, size_t hlen, const char *nee, size_t nlen,
                      int icase, size_t from, size_t *at) {
    if (nlen == 0) { *at = from; return from <= hlen; }
    if (nlen > hlen) return 0;
    for (size_t i = from; i + nlen <= hlen; i++) {
        if (str_eq(hay + i, nee, nlen, icase)) { *at = i; return 1; }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* pattern compilation                                                 */
/* ------------------------------------------------------------------ */
static char *anchor_x(const char *p, int extended) {
    size_t n = strlen(p);
    char *out = xmalloc(n + 8);
    if (extended) snprintf(out, n + 8, "^(?:%s)$", p);
    else          snprintf(out, n + 8, "^\\(%s\\)$", p);
    return out;
}

static int add_pattern_text(const char *text) {
    if (g_npat >= g_cpat) {
        g_cpat = g_cpat ? g_cpat * 2 : 8;
        g_pat = xrealloc(g_pat, sizeof(pat_t) * (size_t)g_cpat);
    }
    pat_t *p = &g_pat[g_npat];
    memset(p, 0, sizeof(*p));
    p->text = xstrdup(text);
    p->len = strlen(text);

    if (o_fixed) {
        p->is_regex = 0;
    } else {
        p->is_regex = 1;
        int cflags = (o_extended ? REG_EXTENDED : 0) | (o_icase ? REG_ICASE : 0);
        char *src = p->text;
        if (o_line) src = anchor_x(p->text, o_extended);
        char ebuf[128];
        int rc = regcomp(&p->re, src, cflags);
        if (o_line) free(src);
        if (rc != 0) {
            regerror(rc, &p->re, ebuf, sizeof(ebuf));
            fprintf(stderr, "%s: %s: %s\n", PROG, p->text, ebuf);
            g_exit = 2;
            return -1;
        }
    }
    g_npat++;
    return 0;
}

static void add_patterns_from_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "%s: %s: %s\n", PROG, path, strerror(errno));
        g_exit = 2;
        return;
    }
    char *buf = NULL;
    size_t len = 0, cap = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') {
            buf[len] = '\0';
            add_pattern_text(buf);
            len = 0;
        } else {
            if (len + 2 > cap) {
                cap = cap ? cap * 2 : 128;
                buf = xrealloc(buf, cap);
            }
            buf[len++] = (char)c;
        }
    }
    if (len > 0) {
        buf[len] = '\0';
        add_pattern_text(buf);
    }
    free(buf);
    fclose(f);
}

/* ------------------------------------------------------------------ */
/* matching primitives                                                 */
/* ------------------------------------------------------------------ */
/* Find the leftmost match of one pattern at or after 'off'. */
static int pat_next(const pat_t *p, const char *line, size_t len, size_t off,
                    size_t *so, size_t *eo) {
    if (off > len) return 0;
    if (!p->is_regex) {
        if (o_line) {  /* -x with fixed strings: whole line must equal pattern */
            if (off == 0 && len == p->len && str_eq(line, p->text, len, o_icase)) {
                *so = 0; *eo = len; return 1;
            }
            return 0;
        }
        size_t at;
        if (!find_fixed(line, len, p->text, p->len, o_icase, off, &at)) return 0;
        *so = at; *eo = at + p->len;
        return 1;
    }
    regmatch_t pm;
    int eflags = (off > 0) ? REG_NOTBOL : 0;
    if (regexec(&p->re, line + off, 1, &pm, eflags) != 0) return 0;
    *so = off + (size_t)pm.rm_so;
    *eo = off + (size_t)pm.rm_eo;
    return 1;
}

/* Leftmost match across all patterns at or after 'off'. */
static int next_match(const char *line, size_t len, size_t off, size_t *so, size_t *eo) {
    int found = 0;
    size_t bso = 0, beo = 0;
    for (int i = 0; i < g_npat; i++) {
        size_t s, e;
        if (pat_next(&g_pat[i], line, len, off, &s, &e)) {
            if (!found || s < bso) { found = 1; bso = s; beo = e; }
        }
    }
    if (found) { *so = bso; *eo = beo; }
    return found;
}

static int word_ok(const char *line, size_t len, size_t so, size_t eo) {
    int before = (so == 0) || !is_wordc((unsigned char)line[so - 1]);
    int after  = (eo >= len) || !is_wordc((unsigned char)line[eo]);
    return before && after;
}

/* Does the line get selected (before -v inversion)? */
static int line_matches(const char *line, size_t len) {
    if (g_npat == 0) return 0;
    size_t off = 0, so, eo;
    while (next_match(line, len, off, &so, &eo)) {
        if (!o_word || word_ok(line, len, so, eo)) return 1;
        off = so + 1;                    /* retry overlapping candidates */
        if (off > len) break;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* argument permutation (EwokOS getopt does not reorder args)          */
/* ------------------------------------------------------------------ */
static const char *g_optstr = "e:f:iym:abcA:B:C:d:D:EgGhHIlLnoqrsUvVwxFRPzTZu";

static struct option g_longopts[] = {
    { "basic-regexp",           no_argument,       0, 'G' },
    { "extended-regexp",        no_argument,       0, 'E' },
    { "perl-regexp",            no_argument,       0, 'P' },
    { "fixed-strings",          no_argument,       0, 'F' },
    { "regexp",                 required_argument, 0, 'e' },
    { "ignore-case",            no_argument,       0, 'i' },
    { "no-ignore-case",         no_argument,       0, 1001 },
    { "word-regexp",            no_argument,       0, 'w' },
    { "line-regexp",            no_argument,       0, 'x' },
    { "file",                   required_argument, 0, 'f' },
    { "max-count",              required_argument, 0, 'm' },
    { "count",                  no_argument,       0, 'c' },
    { "files-without-match",    no_argument,       0, 'L' },
    { "files-with-matches",     no_argument,       0, 'l' },
    { "only-matching",          no_argument,       0, 'o' },
    { "quiet",                  no_argument,       0, 'q' },
    { "silent",                 no_argument,       0, 'q' },
    { "no-messages",            no_argument,       0, 's' },
    { "invert-match",           no_argument,       0, 'v' },
    { "binary-files",           required_argument, 0, 1002 },
    { "text",                   no_argument,       0, 'a' },
    { "binary",                 no_argument,       0, 'U' },
    { "ignore-binary",          no_argument,       0, 'I' },
    { "null-data",              no_argument,       0, 'z' },
    { "after-context",          required_argument, 0, 'A' },
    { "before-context",         required_argument, 0, 'B' },
    { "context",                required_argument, 0, 'C' },
    { "group-separator",        required_argument, 0, 1003 },
    { "no-group-separator",     no_argument,       0, 1004 },
    { "byte-offset",            no_argument,       0, 'b' },
    { "color",                  optional_argument, 0, 1005 },
    { "colour",                 optional_argument, 0, 1005 },
    { "line-buffered",          no_argument,       0, 1006 },
    { "with-filename",          no_argument,       0, 'H' },
    { "without-filename",       no_argument,       0, 'h' },
    { "label",                  required_argument, 0, 1007 },
    { "line-number",            no_argument,       0, 'n' },
    { "initial-tab",            no_argument,       0, 'T' },
    { "null",                   no_argument,       0, 'Z' },
    { "unix-byte-offsets",      no_argument,       0, 'u' },
    { "recursive",              no_argument,       0, 'r' },
    { "dereference-recursive",  no_argument,       0, 'R' },
    { "directories",            required_argument, 0, 'd' },
    { "devices",                required_argument, 0, 'D' },
    { "exclude",                required_argument, 0, 1008 },
    { "exclude-from",           required_argument, 0, 1009 },
    { "exclude-dir",            required_argument, 0, 1010 },
    { "include",                required_argument, 0, 1011 },
    { "help",                   no_argument,       0, 1012 },
    { "version",                no_argument,       0, 'V' },
    { 0, 0, 0, 0 }
};

static int long_needs_next(const char *tok) {
    const char *name = tok + 2;
    const char *eq = strchr(name, '=');
    size_t nlen = eq ? (size_t)(eq - name) : strlen(name);
    for (int i = 0; g_longopts[i].name; i++) {
        if (strncmp(g_longopts[i].name, name, nlen) == 0 && g_longopts[i].name[nlen] == '\0') {
            if (eq) return 0;
            return g_longopts[i].has_arg == required_argument ? 1 : 0;
        }
    }
    return 0;
}

static int short_needs_next(const char *tok) {
    for (const char *p = tok + 1; *p; p++) {
        const char *d = strchr(g_optstr, *p);
        if (d && d[1] == ':') return p[1] == '\0' ? 1 : 0;
    }
    return 0;
}

/* Reorder argv so every option precedes the operands (GNU getopt behaviour). */
static char **permute_argv(int argc, char **argv, int *out_argc) {
    char **opt = xmalloc(sizeof(char *) * (size_t)(argc + 2));
    char **pos = xmalloc(sizeof(char *) * (size_t)(argc + 2));
    int nopt = 0, npos = 0, dashdash = 0;
    opt[nopt++] = argv[0];
    for (int i = 1; i < argc; ) {
        char *t = argv[i];
        if (dashdash) { pos[npos++] = t; i++; continue; }
        if (strcmp(t, "--") == 0) { dashdash = 1; i++; continue; }
        if (t[0] == '-' && t[1] == '-' && t[2] != '\0') {
            opt[nopt++] = t; i++;
            if (long_needs_next(t) && i < argc) { opt[nopt++] = argv[i]; i++; }
            continue;
        }
        if (t[0] == '-' && t[1] != '\0') {
            opt[nopt++] = t; i++;
            if (short_needs_next(t) && i < argc) { opt[nopt++] = argv[i]; i++; }
            continue;
        }
        pos[npos++] = t; i++;
    }
    char **nargv = xmalloc(sizeof(char *) * (size_t)(argc + 2));
    int k = 0;
    for (int j = 0; j < nopt; j++) nargv[k++] = opt[j];
    if (npos > 0) nargv[k++] = (char *)"--";
    for (int j = 0; j < npos; j++) nargv[k++] = pos[j];
    nargv[k] = NULL;
    free(opt); free(pos);
    *out_argc = k;
    return nargv;
}

/* ------------------------------------------------------------------ */
/* line reader                                                         */
/* ------------------------------------------------------------------ */
typedef struct {
    int    fd;
    char   rbuf[READ_CHUNK];
    size_t rlen, rpos;
    char  *line; size_t llen, lcap;
    off_t  bpos;                 /* byte offset of the next unconsumed byte */
    off_t  boff;                 /* byte offset of the current line start */
    unsigned long lineno;
    int    eof;
    int    binary;               /* a NUL was seen (text mode only) */
} rdr_t;

static int rdr_line(rdr_t *r, const char **out, size_t *outlen) {
    char sep = o_null_data ? '\0' : '\n';
    int got = 0;
    r->llen = 0;
    r->boff = r->bpos;
    for (;;) {
        if (r->rpos >= r->rlen) {
            ssize_t n;
            do { n = read(r->fd, r->rbuf, sizeof(r->rbuf)); } while (n < 0 && errno == EAGAIN);
            if (n <= 0) { r->eof = 1; break; }
            r->rlen = (size_t)n;
            r->rpos = 0;
        }
        size_t start = r->rpos;
        while (r->rpos < r->rlen && r->rbuf[r->rpos] != sep) {
            if (!o_null_data && r->rbuf[r->rpos] == '\0') r->binary = 1;
            r->rpos++;
        }
        if (r->llen + (r->rpos - start) + 1 > r->lcap) {
            r->lcap = r->llen + (r->rpos - start) + 1;
            r->line = xrealloc(r->line, r->lcap);
        }
        memcpy(r->line + r->llen, r->rbuf + start, r->rpos - start);
        r->llen += r->rpos - start;
        r->bpos += (off_t)(r->rpos - start);
        got = 1;
        if (r->rpos < r->rlen) {  /* separator found */
            r->rpos++;
            r->bpos++;
            break;
        }
    }
    if (!got) return 0;
    r->line[r->llen] = '\0';
    *out = r->line;
    *outlen = r->llen;
    return 1;
}

/* ------------------------------------------------------------------ */
/* output helpers                                                      */
/* ------------------------------------------------------------------ */
static void emit_prefix(const char *fn, unsigned long lineno, off_t boff, char sep) {
    if (g_show_fn && fn) {
        col(C_FN);
        fputs(fn, stdout);
        col(C_OFF);
        col(C_SEP);
        fputc(o_null_fn ? '\0' : sep, stdout);
        col(C_OFF);
    }
    if (o_lineno) {
        col(C_LN);
        printf("%lu", lineno);
        col(C_OFF);
        col(C_SEP);
        fputc(sep, stdout);
        col(C_OFF);
    }
    if (o_byteoff) {
        col(C_LN);
        printf("%lld", (long long)boff);
        col(C_OFF);
        col(C_SEP);
        fputc(sep, stdout);
        col(C_OFF);
    }
    if (o_init_tab) fputc('\t', stdout);
}

static void emit_newline(void) {
    fputc(o_null_data ? '\0' : '\n', stdout);
    if (o_line_buf) fflush(stdout);
}

static void emit_fname(const char *fn) {
    fputs(fn, stdout);
    fputc(o_null_fn ? '\0' : '\n', stdout);
    if (o_line_buf) fflush(stdout);
}

static void emit_plain(const char *line, size_t len) {
    fwrite(line, 1, len, stdout);
    emit_newline();
}

static void emit_colored(const char *line, size_t len, int is_ctx) {
    size_t off = 0, so, eo, last = 0;
    while (next_match(line, len, off, &so, &eo)) {
        if (o_word && !word_ok(line, len, so, eo)) {
            off = so + 1;                /* retry overlapping candidates */
            if (off > len) break;
            continue;
        }
        fwrite(line + last, 1, so - last, stdout);
        col(is_ctx ? C_CTX : C_SEL);
        fwrite(line + so, 1, eo - so, stdout);
        col(C_OFF);
        last = eo;
        off = (eo > so) ? eo : so + 1;
        if (off > len) break;
    }
    fwrite(line + last, 1, len - last, stdout);
    emit_newline();
}

static void emit_matches_only(const char *fn, const char *line, size_t len,
                              unsigned long lineno, off_t boff) {
    size_t off = 0, so, eo;
    while (next_match(line, len, off, &so, &eo)) {
        if (o_word && !word_ok(line, len, so, eo)) {
            off = so + 1;                /* retry overlapping candidates */
            if (off > len) break;
            continue;
        }
        emit_prefix(fn, lineno, boff + (off_t)so, ':');
        col(C_SEL);
        fwrite(line + so, 1, eo - so, stdout);
        col(C_OFF);
        emit_newline();
        off = (eo > so) ? eo : so + 1;
        if (off > len) break;
    }
}

/* before-context ring (-B) */
typedef struct {
    char  *text;
    size_t len;
    off_t  boff;
    unsigned long lineno;
} ctx_t;
static ctx_t *g_ring;
static int g_ring_n, g_ring_cap;

static void ring_push(const char *line, size_t len, off_t boff, unsigned long lineno) {
    if (g_ring_n > 0 && g_ring_n >= o_before) {
        free(g_ring[0].text);
        memmove(g_ring, g_ring + 1, sizeof(ctx_t) * (size_t)(g_ring_n - 1));
        g_ring_n--;
    }
    if (g_ring_n >= g_ring_cap) {
        g_ring_cap = g_ring_cap ? g_ring_cap * 2 : 8;
        g_ring = xrealloc(g_ring, sizeof(ctx_t) * (size_t)g_ring_cap);
    }
    ctx_t *c = &g_ring[g_ring_n++];
    c->text = xmalloc(len ? len : 1);
    memcpy(c->text, line, len);
    c->len = len;
    c->boff = boff;
    c->lineno = lineno;
}
static void ring_clear(void) {
    for (int i = 0; i < g_ring_n; i++) free(g_ring[i].text);
    g_ring_n = 0;
}

/* ------------------------------------------------------------------ */
/* per-file scan state                                                 */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *fn;
    long long sel_count;       /* selected lines in this file */
    unsigned long last_printed;
    int pending_after;         /* trailing context lines still to print */
    int any_match;
    int stop;                  /* -m limit reached */
} scan_t;

static void maybe_group_sep(scan_t *st, unsigned long lineno) {
    if (!o_show_group_sep || (o_after == 0 && o_before == 0)) return;
    if (st->last_printed != 0 && lineno != st->last_printed + 1) {
        fputs(o_group_sep, stdout);
        emit_newline();
    }
}

static void emit_context_line(const char *fn, const char *line, size_t len,
                              off_t boff, unsigned long lineno) {
    emit_prefix(fn, lineno, boff, '-');
    if (o_use_color) emit_colored(line, len, 1);
    else emit_plain(line, len);
}

static void emit_sel_line(const char *fn, const char *line, size_t len,
                          off_t boff, unsigned long lineno) {
    emit_prefix(fn, lineno, boff, ':');
    if (o_use_color) emit_colored(line, len, 0);
    else emit_plain(line, len);
}

/* ------------------------------------------------------------------ */
/* file scanning                                                       */
/* ------------------------------------------------------------------ */
static void flush_before_ctx(scan_t *st) {
    for (int i = 0; i < g_ring_n; i++) {
        if (g_ring[i].lineno > st->last_printed) {
            maybe_group_sep(st, g_ring[i].lineno);
            emit_context_line(st->fn, g_ring[i].text, g_ring[i].len,
                              g_ring[i].boff, g_ring[i].lineno);
            st->last_printed = g_ring[i].lineno;
        }
    }
    ring_clear();
}

/* Returns 1 if the line was selected. */
static int handle_line(scan_t *st, const char *line, size_t len,
                       off_t boff, unsigned long lineno, int is_match) {
    int selected = o_invert ? !is_match : is_match;

    if (selected) {
        if (o_max_count >= 0 && g_matched_lines >= o_max_count) {
            st->stop = 1;
            return 0;
        }
        st->sel_count++;
        g_matched_lines++;
        st->any_match = 1;
        /* count / filename / quiet modes: no line or context output */
        if (o_count || o_files_w || o_files_wo || o_quiet) return 1;
        flush_before_ctx(st);
        if (o_only_matching) {
            maybe_group_sep(st, lineno);
            emit_matches_only(st->fn, line, len, lineno, boff);
        } else {
            maybe_group_sep(st, lineno);
            emit_sel_line(st->fn, line, len, boff, lineno);
        }
        st->last_printed = lineno;
        st->pending_after = o_after;
        return 1;
    }
    if (o_count || o_files_w || o_files_wo || o_quiet) return 0;
    if (st->pending_after > 0) {
        maybe_group_sep(st, lineno);
        emit_context_line(st->fn, line, len, boff, lineno);
        st->last_printed = lineno;
        st->pending_after--;
    } else if (o_before > 0) {
        ring_push(line, len, boff, lineno);
    }
    return 0;
}

/* Scan one already-open input. fn is the display name (never NULL). */
static void grep_fd(const char *fn, int fd, int close_fd) {
    rdr_t r;
    memset(&r, 0, sizeof(r));
    r.fd = fd;
    scan_t st;
    memset(&st, 0, sizeof(st));
    st.fn = fn;

    const char *line;
    size_t len;
    while (!st.stop && rdr_line(&r, &line, &len)) {
        r.lineno++;
        if (!o_text && r.binary) {
            if (o_ignore_bin) break;             /* -I: no match contribution */
            if (st.any_match) {                  /* matched before the binary part */
                if (g_exit != 2) g_exit = 0;
                if (!o_quiet && !o_count && !o_files_w && !o_files_wo)
                    printf("Binary file %s matches\n", fn);
                break;
            }
            int is_match = line_matches(line, len);
            if (!(o_invert ? !is_match : is_match)) continue;  /* not selected */
            /* first selected line inside binary data: report and stop */
            st.sel_count++;
            g_matched_lines++;
            st.any_match = 1;
            if (g_exit != 2) g_exit = 0;
            if (o_quiet || o_files_wo) break;
            if (o_files_w) { emit_fname(fn); break; }
            if (o_count) {
                if (g_show_fn) printf("%s:", fn);
                printf("%lld", st.sel_count);
                emit_newline();
            } else {
                printf("Binary file %s matches\n", fn);
            }
            break;
        }
        int is_match = line_matches(line, len);
        int sel = handle_line(&st, line, len, r.boff, r.lineno, is_match);
        if (sel) {
            if (g_exit != 2) g_exit = 0;
            if (o_quiet || o_files_w) {
                if (o_files_w && !o_quiet) emit_fname(fn);
                break;
            }
        }
    }

    if (o_files_wo && !o_quiet && !st.any_match) emit_fname(fn);
    if (o_count && !o_quiet && !o_files_w && !o_files_wo) {
        if (g_show_fn) {
            col(C_FN);
            fputs(fn, stdout);
            col(C_OFF);
            col(C_SEP);
            fputc(':', stdout);
            col(C_OFF);
        }
        printf("%lld", st.sel_count);
        emit_newline();
    }
    if (close_fd) close(fd);
}

/* ------------------------------------------------------------------ */
/* directory traversal                                                 */
/* ------------------------------------------------------------------ */
static int in_list(char **list, int n, const char *name) {
    for (int i = 0; i < n; i++)
        if (fnmatch(list[i], name, 0) == 0) return 1;
    return 0;
}

static const char *base_name(const char *p) {
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

static int file_included(const char *base) {
    if (g_nexc && in_list(g_exc, g_nexc, base)) return 0;
    if (g_ninc && !in_list(g_inc, g_ninc, base)) return 0;
    return 1;
}

static void grep_named_file(const char *path, const char *display) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (!o_no_msgs) fprintf(stderr, "%s: %s: %s\n", PROG, path, strerror(errno));
        g_exit = 2;
        return;
    }
    grep_fd(display, fd, 1);
}

static void search_dir(const char *path, const char *display) {
    DIR *d = opendir(path);
    if (!d) {
        if (!o_no_msgs) fprintf(stderr, "%s: %s: %s\n", PROG, path, strerror(errno));
        g_exit = 2;
        return;
    }
    size_t plen = strlen(path);
    size_t dlen = strlen(display);
    char *cpath = xmalloc(plen + 2);
    char *cdisp = xmalloc(dlen + 2);
    memcpy(cpath, path, plen);
    memcpy(cdisp, display, dlen);
    if (plen == 0 || cpath[plen - 1] != '/') cpath[plen++] = '/';
    if (dlen == 0 || cdisp[dlen - 1] != '/') cdisp[dlen++] = '/';

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        const char *nm = de->d_name;
        if (strcmp(nm, ".") == 0 || strcmp(nm, "..") == 0) continue;
        size_t nlen = strlen(nm);
        cpath = xrealloc(cpath, plen + nlen + 2);
        cdisp = xrealloc(cdisp, dlen + nlen + 2);
        memcpy(cpath + plen, nm, nlen + 1);
        memcpy(cdisp + dlen, nm, nlen + 1);

        struct stat st;
        if (stat(cpath, &st) != 0) {
            if (!o_no_msgs) fprintf(stderr, "%s: %s: %s\n", PROG, cpath, strerror(errno));
            g_exit = 2;
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            if (g_nexcdir && in_list(g_excdir, g_nexcdir, nm)) continue;
            search_dir(cpath, cdisp);
        } else if (S_ISREG(st.st_mode)) {
            if (file_included(nm)) grep_named_file(cpath, cdisp);
        }
        /* devices/symlinks: -R follows symlinks via stat(); -r cannot be
         * distinguished from -R here (EwokOS lstat()==stat()). */
    }
    free(cpath);
    free(cdisp);
    closedir(d);
}

static void process_arg(const char *arg) {
    struct stat st;
    if (stat(arg, &st) != 0) {
        if (!o_no_msgs) fprintf(stderr, "%s: %s: %s\n", PROG, arg, strerror(errno));
        g_exit = 2;
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        if (o_recursive || o_deref_rec || o_dir_action == 2) {
            search_dir(arg, arg);
        } else if (o_dir_action == 1) {
            return; /* skip */
        } else {
            if (!o_no_msgs) fprintf(stderr, "%s: %s: Is a directory\n", PROG, arg);
            g_exit = 2;
        }
        return;
    }
    if (!S_ISREG(st.st_mode)) {
        if (o_dev_action == 1) return; /* skip devices */
        grep_named_file(arg, arg);
        return;
    }
    if (!file_included(base_name(arg))) return;
    grep_named_file(arg, arg);
}

/* ------------------------------------------------------------------ */
/* usage / help                                                        */
/* ------------------------------------------------------------------ */
static void usage(FILE *f) {
    fprintf(f,
        "Usage: %s [OPTION]... PATTERNS [FILE]...\n"
        "Search for PATTERNS in each FILE.  If no FILE, read standard input.\n"
        "Example: %s -i 'hello world' menu.h main.c\n"
        "\n"
        "Pattern selection and interpretation:\n"
        "  -E, --extended-regexp     PATTERNS are extended regular expressions\n"
        "  -F, --fixed-strings       PATTERNS are literal strings\n"
        "  -G, --basic-regexp        PATTERNS are basic regular expressions (default)\n"
        "  -P, --perl-regexp         accepted for compatibility, acts like -E\n"
        "  -e, --regexp=PATTERNS     use PATTERNS for matching\n"
        "  -f, --file=FILE           take PATTERNS from FILE\n"
        "  -i, --ignore-case         ignore case distinctions in patterns and data\n"
        "      --no-ignore-case      do not ignore case (default)\n"
        "  -v, --invert-match        select non-matching lines\n"
        "  -w, --word-regexp         match only whole words\n"
        "  -x, --line-regexp         match only whole lines\n"
        "\n"
        "Miscellaneous:\n"
        "  -s, --no-messages         suppress error messages about unreadable files\n"
        "  -r, --recursive           search directories recursively\n"
        "  -R, --dereference-recursive  likewise, following all symlinks\n"
        "  -d, --directories=ACTION  how to handle directories: read, recurse, skip\n"
        "  -D, --devices=ACTION      how to handle devices, FIFOs and sockets: read, skip\n"
        "      --include=GLOB        search only files whose base name matches GLOB\n"
        "      --exclude=GLOB        skip files whose base name matches GLOB\n"
        "      --exclude-from=FILE   skip files matching any GLOB in FILE\n"
        "      --exclude-dir=GLOB    skip directories whose base name matches GLOB\n"
        "  -a, --text                process a binary file as if it were text\n"
        "  -I                        equivalent to --binary-files=without-match\n"
        "      --binary-files=TYPE   assume binary TYPE: binary, text, without-match\n"
        "  -z, --null-data           a data line ends in 0 byte, not newline\n"
        "  -m, --max-count=NUM       stop after NUM selected lines\n"
        "      --line-buffered       flush output on every line\n"
        "\n"
        "Output control:\n"
        "  -o, --only-matching       show only nonempty parts of lines that match\n"
        "  -q, --quiet, --silent     suppress all normal output\n"
        "  -c, --count               print only a count of selected lines per FILE\n"
        "  -l, --files-with-matches  print only names of FILEs with selected lines\n"
        "  -L, --files-without-match print only names of FILEs with no selected lines\n"
        "  -A, --after-context=NUM   print NUM lines of trailing context\n"
        "  -B, --before-context=NUM  print NUM lines of leading context\n"
        "  -C, --context=NUM         print NUM lines of output context\n"
        "      --group-separator=SEP print SEP on line between context groups\n"
        "      --no-group-separator  do not print the separator line\n"
        "  -b, --byte-offset         print the byte offset with output lines\n"
        "  -n, --line-number         print line number with output lines\n"
        "  -H, --with-filename       print file name with output lines\n"
        "  -h, --without-filename    suppress the file name prefix on output\n"
        "  -T, --initial-tab         make tabs line up (prefix each line with a TAB)\n"
        "  -Z, --null                print 0 byte after FILE name\n"
        "      --label=LABEL         use LABEL as the standard input file name\n"
        "  -u, --unix-byte-offsets   accepted for compatibility (offsets are already Unix-style)\n"
        "      --color[=WHEN], --colour[=WHEN]  colorize output: always, never, auto (default)\n"
        "  -V, --version             print version information and exit\n"
        "      --help                display this help and exit\n"
        "\n"
        "Exit status: 0 if a line is selected, 1 if no lines selected, 2 on error.\n",
        PROG, PROG);
}

/* ------------------------------------------------------------------ */
/* option parsing                                                      */
/* ------------------------------------------------------------------ */
static long parse_num(const char *s, int *ok) {
    char *end;
    long v = strtol(s, &end, 10);
    *ok = (end != s && *end == '\0');
    return v;
}

static void set_dir_action(const char *a) {
    if (strcmp(a, "read") == 0) o_dir_action = 0;
    else if (strcmp(a, "skip") == 0) o_dir_action = 1;
    else if (strcmp(a, "recurse") == 0) o_dir_action = 2;
    else {
        fprintf(stderr, "%s: invalid argument '%s' for --directories\n", PROG, a);
        g_exit = 2;
    }
}

static void set_dev_action(const char *a) {
    if (strcmp(a, "read") == 0) o_dev_action = 0;
    else if (strcmp(a, "skip") == 0) o_dev_action = 1;
    else {
        fprintf(stderr, "%s: invalid argument '%s' for --devices\n", PROG, a);
        g_exit = 2;
    }
}

int main(int argc, char **argv) {
    int nargc;
    char **nargv = permute_argv(argc, argv, &nargc);

    int got_epat = 0;
    int opt;
    int err = 0;
    int idx = 0;
    while ((opt = getopt_long(nargc, nargv, g_optstr, g_longopts, &idx)) != -1) {
        switch (opt) {
        case 'e': got_epat = 1; add_pattern_text(optarg); break;
        case 'f': got_epat = 1; add_patterns_from_file(optarg); break;
        case 'i': case 'y': o_icase = 1; break;
        case 1001: o_icase = 0; break;
        case 'v': o_invert = 1; break;
        case 'w': o_word = 1; break;
        case 'x': o_line = 1; break;
        case 'E': case 'P': o_extended = 1; break;
        case 'G': case 'g': o_extended = 0; break;
        case 'F': o_fixed = 1; break;
        case 'm': {
            int ok;
            long v = parse_num(optarg, &ok);
            if (!ok || v < 0) { fprintf(stderr, "%s: invalid max count\n", PROG); g_exit = 2; }
            else o_max_count = (long long)v;
            break;
        }
        case 'c': o_count = 1; break;
        case 'l': o_files_w = 1; break;
        case 'L': o_files_wo = 1; break;
        case 'o': o_only_matching = 1; break;
        case 'q': o_quiet = 1; break;
        case 's': o_no_msgs = 1; break;
        case 'n': o_lineno = 1; break;
        case 'b': o_byteoff = 1; break;
        case 'H': o_with_fn = 1; break;
        case 'h': o_without_fn = 1; break;
        case 'T': o_init_tab = 1; break;
        case 'Z': o_null_fn = 1; break;
        case 'z': o_null_data = 1; break;
        case 'a': o_text = 1; break;
        case 'I': o_ignore_bin = 1; break;
        case 'U': case 'u': break; /* no-op on Unix */
        case 'r': o_recursive = 1; break;
        case 'R': o_deref_rec = 1; o_recursive = 1; break;
        case 'd': set_dir_action(optarg); break;
        case 'D': set_dev_action(optarg); break;
        case 'A': {
            int ok;
            long v = parse_num(optarg, &ok);
            if (!ok || v < 0) { fprintf(stderr, "%s: invalid context length argument\n", PROG); g_exit = 2; }
            else o_after = (int)v;
            break;
        }
        case 'B': {
            int ok;
            long v = parse_num(optarg, &ok);
            if (!ok || v < 0) { fprintf(stderr, "%s: invalid context length argument\n", PROG); g_exit = 2; }
            else o_before = (int)v;
            break;
        }
        case 'C': {
            int ok;
            long v = parse_num(optarg, &ok);
            if (!ok || v < 0) { fprintf(stderr, "%s: invalid context length argument\n", PROG); g_exit = 2; }
            else { o_after = (int)v; o_before = (int)v; }
            break;
        }
        case 1002: /* --binary-files */
            if (strcmp(optarg, "binary") == 0) { o_text = 0; o_ignore_bin = 0; }
            else if (strcmp(optarg, "text") == 0) { o_text = 1; o_ignore_bin = 0; }
            else if (strcmp(optarg, "without-match") == 0) { o_ignore_bin = 1; o_text = 0; }
            else {
                fprintf(stderr, "%s: invalid argument '%s' for --binary-files\n", PROG, optarg);
                g_exit = 2;
            }
            break;
        case 1003: o_group_sep = optarg; break;
        case 1004: o_show_group_sep = 0; break;
        case 1005: /* --color[=WHEN] */
            if (!optarg || strcmp(optarg, "always") == 0 || strcmp(optarg, "yes") == 0 ||
                strcmp(optarg, "force") == 0)
                o_color = 1;
            else if (strcmp(optarg, "auto") == 0 || strcmp(optarg, "tty") == 0 ||
                     strcmp(optarg, "if-tty") == 0)
                o_color = 2;
            else if (strcmp(optarg, "never") == 0 || strcmp(optarg, "no") == 0 ||
                     strcmp(optarg, "none") == 0)
                o_color = 0;
            else {
                fprintf(stderr, "%s: invalid argument '%s' for --color\n", PROG, optarg);
                g_exit = 2;
            }
            break;
        case 1006: o_line_buf = 1; break;
        case 1007: o_label = optarg; break;
        case 1008: list_add(&g_exc, &g_nexc, NULL, optarg); break;
        case 1009: {
            FILE *f = fopen(optarg, "r");
            if (!f) {
                fprintf(stderr, "%s: %s: %s\n", PROG, optarg, strerror(errno));
                g_exit = 2;
                break;
            }
            char *buf = NULL;
            size_t len = 0, cap = 0;
            int c;
            while ((c = fgetc(f)) != EOF) {
                if (c == '\n') {
                    buf[len] = '\0';
                    if (len > 0) list_add(&g_exc, &g_nexc, NULL, buf);
                    len = 0;
                } else {
                    if (len + 2 > cap) { cap = cap ? cap * 2 : 128; buf = xrealloc(buf, cap); }
                    buf[len++] = (char)c;
                }
            }
            if (len > 0) { buf[len] = '\0'; list_add(&g_exc, &g_nexc, NULL, buf); }
            free(buf);
            fclose(f);
            break;
        }
        case 1010: list_add(&g_excdir, &g_nexcdir, NULL, optarg); break;
        case 1011: list_add(&g_inc, &g_ninc, NULL, optarg); break;
        case 1012: usage(stdout); exit(0); break;
        case 'V':
            printf("%s (EwokOS) 3.11-compatible\n", PROG);
            exit(0);
            break;
        case '?':
        default:
            err = 1;
            break;
        }
    }

    /* the first operand is the pattern unless -e/-f was given */
    if (g_npat == 0) {
        if (optind < nargc && !got_epat) {
            add_pattern_text(nargv[optind++]);
        } else if (!got_epat) {
            fprintf(stderr, "%s: no pattern given\n", PROG);
            usage(stderr);
            exit(2);
        }
    }

    /* compile-time flag consistency is already resolved; patterns compiled above */
    if (err) {
        usage(stderr);
        exit(2);
    }
    if (g_exit == 2) exit(2);

    if (o_with_fn && o_without_fn) {
        fprintf(stderr, "%s: options -H and -h are mutually exclusive\n", PROG);
        exit(2);
    }

    o_use_color = (o_color == 1) || (o_color == 2 && isatty(1));

    /* resolve filename prefix default */
    int nfiles = nargc - optind;
    g_show_fn = o_with_fn ? 1
              : o_without_fn ? 0
              : (o_recursive || o_deref_rec || nfiles > 1);

    if (nfiles == 0) {
        grep_fd(o_label ? o_label : STDIN_NAME, 0, 0);
    } else {
        for (int i = optind; i < nargc; i++) {
            if (strcmp(nargv[i], "-") == 0)
                grep_fd(o_label ? o_label : STDIN_NAME, 0, 0);
            else
                process_arg(nargv[i]);
        }
    }
    fflush(stdout);
    return g_exit;
}
