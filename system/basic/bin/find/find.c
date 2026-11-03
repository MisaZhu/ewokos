/*
 * find - search for files in a directory hierarchy.
 *
 * GNU find compatible subset:
 *   find [-H|-L|-P] [path...] [expression]
 *
 * Tests   : -name -iname -path -ipath -wholename -type -empty -size -links
 *           -mtime -atime -ctime -mmin -amin -cmin -newer -perm -user -group
 *           -true -false
 * Options : -maxdepth -mindepth -depth -xdev -mount -follow
 * Actions : -print -print0 -printf -prune -quit -delete -exec ... \; -exec ... +
 *           -ok ... \;
 * Ops     : ! / -not, -a / -and (and juxtaposition), -o / -or, ( ... )
 *
 * Platform notes (EwokOS):
 *  - lstat() is an alias of stat(), so -type l only matches when the VFS
 *    reports a link; -H/-L/-P are accepted for compatibility.
 *  - waitpid() does not report a child's exit status, so -exec/-ok always
 *    evaluate true once the command has been launched.
 *  - the kernel command line is capped at PROC_INFO_MAX_CMD_LEN (256) bytes,
 *    which bounds how much an "-exec ... +" batch can carry.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fnmatch.h>
#include <ewoksys/session.h>

#define FIND_MAX_PATH    FS_FULL_NAME_MAX   /* 512 */
/* Keep -exec/+ batches well under the 256 byte kernel command-line cap. */
#define EXEC_CMD_BUDGET  200
#define MAX_PLUS_NODES   32

/* ------------------------------------------------------------------ */
/* expression AST                                                      */
/* ------------------------------------------------------------------ */
enum {
    K_AND, K_OR, K_NOT,
    /* tests */
    K_NAME, K_INAME, K_PATH, K_IPATH,
    K_TYPE, K_EMPTY, K_SIZE, K_LINKS, K_TIME,
    K_NEWER, K_PERM, K_USER, K_GROUP, K_TRUE, K_FALSE,
    /* actions */
    K_PRINT, K_PRINT0, K_PRINTF, K_PRUNE, K_QUIT, K_DELETE, K_EXEC, K_OK
};

enum { T_ATIME, T_MTIME, T_CTIME };      /* K_TIME: which stamp        */
enum { U_DAY, U_MIN };                   /* K_TIME: op2 unit           */
enum { PERM_EXACT, PERM_ANY, PERM_ALL };  /* K_PERM: op match kind      */

typedef struct node node_t;
struct node {
    int   kind;
    int   is_action;      /* counts against the implicit -print        */
    node_t *a, *b;        /* K_AND / K_OR / K_NOT                      */
    char  *s;             /* pattern / printf format / newer reference */
    /* -exec / -ok */
    char **argv; int argc; int plus;
    char **batch; int nbatch; int batch_cap; int batch_bytes; int flen;
    /* numeric predicates */
    long long num;        /* value (or size unit / perm mode in num2)  */
    int  sign;            /* -1 '<'  0 '=='  +1 '>'                    */
    long long num2;       /* size unit bytes / perm mask / newer mtime */
    int  op;              /* type char / time field / perm kind        */
    int  op2;             /* time unit                                 */
};

typedef struct {
    const char *path;
    const char *base;
    struct stat st;
    int depth;
} ctx_t;

typedef struct {
    int quit;
    int prune;
} state_t;

/* ------------------------------------------------------------------ */
/* globals                                                             */
/* ------------------------------------------------------------------ */
static node_t *g_root = NULL;
static int g_maxdepth = -1;
static int g_mindepth = 0;
static int g_depth = 0;      /* -depth (post-order)                    */
static int g_xdev = 0;       /* -xdev / -mount                         */
static int g_follow = 0;     /* -P(0) -H(1) -L(2); cosmetic on EwokOS  */
static int g_exit = 0;       /* process exit status                    */
static node_t *g_plus[MAX_PLUS_NODES];
static int g_nplus = 0;

static char **g_argv;
static int g_argc;
static int g_pi;

/* ------------------------------------------------------------------ */
/* small helpers                                                       */
/* ------------------------------------------------------------------ */
static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if(p == NULL) { fprintf(stderr, "find: out of memory\n"); exit(2); }
    return p;
}

static char *xstrdup(const char *s) {
    char *p = strdup(s);
    if(p == NULL) { fprintf(stderr, "find: out of memory\n"); exit(2); }
    return p;
}

static node_t *node_new(int kind) {
    node_t *n = (node_t *)xmalloc(sizeof(node_t));
    memset(n, 0, sizeof(*n));
    n->kind = kind;
    n->sign = 0;
    return n;
}

static void die(const char *msg) {
    fprintf(stderr, "find: %s\n", msg);
    exit(1);
}

static void die_arg(const char *pred, const char *why) {
    fprintf(stderr, "find: %s: %s\n", pred, why);
    exit(1);
}

static char *lower(const char *s) {
    char *p = xstrdup(s);
    for(char *q = p; *q; q++)
        if(*q >= 'A' && *q <= 'Z') *q = (char)(*q + 32);
    return p;
}

/* basename of a path (last '/'-separated component, trailing '/' ignored) */
static const char *base_of(const char *path) {
    size_t len = strlen(path);
    while(len > 1 && path[len-1] == '/') len--;
    if(len == 0) return path;
    if(len == 1 && path[0] == '/') return path;
    const char *end = path + len;
    const char *p = end;
    while(p > path && *(p-1) != '/') p--;
    return p;
}

static int glob_match(const char *pat, const char *str, int ci) {
    if(!ci)
        return fnmatch(pat, str, 0) == 0;
    char *p = lower(pat), *s = lower(str);
    int r = fnmatch(p, s, 0) == 0;
    free(p); free(s);
    return r;
}

static int cmp_ok(long long val, long long target, int sign) {
    if(sign < 0) return val < target;
    if(sign > 0) return val > target;
    return val == target;
}

static int type_char_is(int c) {
    return c=='f'||c=='d'||c=='l'||c=='c'||c=='b'||c=='p'||c=='s';
}

static int type_match(int c, mode_t m) {
    switch(c) {
    case 'f': return S_ISREG(m);
    case 'd': return S_ISDIR(m);
    case 'l': return S_ISLNK(m);
    case 'c': return S_ISCHR(m);
    case 'b': return S_ISBLK(m);
    case 'p': return S_ISFIFO(m);
    case 's': return S_ISSOCK(m);
    }
    return 0;
}

static char type_char(mode_t m) {
    if(S_ISDIR(m))  return 'd';
    if(S_ISREG(m))  return 'f';
    if(S_ISLNK(m))  return 'l';
    if(S_ISCHR(m))  return 'c';
    if(S_ISBLK(m))  return 'b';
    if(S_ISFIFO(m)) return 'p';
    if(S_ISSOCK(m)) return 's';
    return '?';
}

static const char *mode_string(mode_t m) {
    static char b[11];
    b[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : S_ISCHR(m) ? 'c' :
           S_ISBLK(m) ? 'b' : S_ISFIFO(m) ? 'p' : S_ISSOCK(m) ? 's' : '-';
    b[1] = (m & 0400) ? 'r' : '-';
    b[2] = (m & 0200) ? 'w' : '-';
    b[3] = (m & 0100) ? 'x' : '-';
    b[4] = (m & 0040) ? 'r' : '-';
    b[5] = (m & 0020) ? 'w' : '-';
    b[6] = (m & 0010) ? 'x' : '-';
    b[7] = (m & 0004) ? 'r' : '-';
    b[8] = (m & 0002) ? 'w' : '-';
    b[9] = (m & 0001) ? 'x' : '-';
    b[10] = 0;
    return b;
}

/* ------------------------------------------------------------------ */
/* numeric argument parsers                                            */
/* ------------------------------------------------------------------ */
/* signed integer used by -size / -links / time tests */
static int parse_signed(const char *s, long long *val, int *sign) {
    int sg = 0;
    if(*s == '+') { sg = 1; s++; }
    else if(*s == '-') { sg = -1; s++; }
    if(*s < '0' || *s > '9') return -1;
    char *e;
    long long v = strtoll(s, &e, 10);
    if(e == s || *e != 0) return -1;
    *val = v; *sign = sg;
    return 0;
}

static int parse_size(const char *s, long long *val, int *sign, long long *unit) {
    int sg = 0;
    if(*s == '+') { sg = 1; s++; }
    else if(*s == '-') { sg = -1; s++; }
    if(*s < '0' || *s > '9') return -1;
    char *e;
    long long v = strtoll(s, &e, 10);
    if(e == s) return -1;
    long long u = 512;                 /* default: 512-byte blocks */
    if(*e) {
        switch(*e) {
        case 'c': u = 1; break;
        case 'w': u = 2; break;
        case 'b': u = 512; break;
        case 'k': case 'K': u = 1024; break;
        case 'M': u = 1024LL * 1024; break;
        case 'G': u = 1024LL * 1024 * 1024; break;
        default: return -1;
        }
        e++;
        if(*e) return -1;
    }
    *val = v; *sign = sg; *unit = u;
    return 0;
}

static int parse_perm(const char *s, long *mode, int *op) {
    *op = PERM_EXACT;
    if(*s == '/')      { *op = PERM_ANY;  s++; }
    else if(*s == '-') { *op = PERM_ALL;  s++; }
    else if(*s == '+') { *op = PERM_ANY;  s++; }
    else if(*s == '=') { s++; }
    if(*s == 0) return -1;
    char *e;
    long v = strtol(s, &e, 8);
    if(e == s || *e != 0) return -1;
    *mode = v;
    return 0;
}

static int is_all_digits(const char *s) {
    if(*s == 0) return 0;
    for(; *s; s++) if(*s < '0' || *s > '9') return 0;
    return 1;
}

static long long resolve_uid(const char *s) {
    if(is_all_digits(s)) return (long long)strtoll(s, NULL, 10);
    session_info_t si;
    if(session_get_by_name(s, &si) == 0) return si.uid;
    return -1;
}

static long long resolve_gid(const char *s) {
    if(is_all_digits(s)) return (long long)strtoll(s, NULL, 10);
    session_group_t gi;
    if(session_get_group_by_name(s, &gi) == 0) return gi.gid;
    return -1;
}

/* ------------------------------------------------------------------ */
/* token helpers                                                       */
/* ------------------------------------------------------------------ */
static const char *peek(void) { return g_pi < g_argc ? g_argv[g_pi] : NULL; }
static const char *next(void) { return g_pi < g_argc ? g_argv[g_pi++] : NULL; }

static int tok_eq(const char *t, const char *w) { return t && strcmp(t, w) == 0; }
static int is_open_paren(const char *t)  { return tok_eq(t,"(")  || tok_eq(t,"\\("); }
static int is_close_paren(const char *t) { return tok_eq(t,")")  || tok_eq(t,"\\)"); }
static int is_bang(const char *t)        { return tok_eq(t,"!")  || tok_eq(t,"\\!"); }

static const char *need_arg(const char *pred) {
    const char *t = next();
    if(t == NULL) die_arg(pred, "expected an argument");
    return t;
}

/* ------------------------------------------------------------------ */
/* parser (recursive descent)                                          */
/* ------------------------------------------------------------------ */
static node_t *parse_expr(void);
static node_t *parse_or(void);
static node_t *parse_and(void);
static node_t *parse_unary(void);
static node_t *parse_primary(void);
static node_t *parse_exec(int kind);

static node_t *parse_expr(void) { return parse_or(); }

static node_t *parse_or(void) {
    node_t *left = parse_and();
    for(;;) {
        const char *t = peek();
        if(!tok_eq(t, "-o") && !tok_eq(t, "-or")) break;
        next();
        node_t *right = parse_and();
        if(right == NULL) die("expected an expression after '-o'");
        node_t *n = node_new(K_OR);
        n->a = left ? left : node_new(K_TRUE);
        n->b = right;
        left = n;
    }
    return left;
}

static node_t *parse_and(void) {
    node_t *left = parse_unary();
    for(;;) {
        const char *t = peek();
        if(t == NULL || is_close_paren(t)) break;
        if(tok_eq(t, "-o") || tok_eq(t, "-or")) break;
        if(tok_eq(t, "-a") || tok_eq(t, "-and")) {
            next();
            node_t *right = parse_unary();
            if(right == NULL) die("expected an expression after '-a'");
            node_t *n = node_new(K_AND);
            n->a = left ? left : node_new(K_TRUE);
            n->b = right;
            left = n;
            continue;
        }
        /* juxtaposition = implicit AND */
        node_t *right = parse_unary();
        if(right == NULL) break;
        node_t *n = node_new(K_AND);
        n->a = left ? left : node_new(K_TRUE);
        n->b = right;
        left = n;
    }
    return left;
}

static node_t *parse_unary(void) {
    const char *t = peek();
    if(t == NULL) return NULL;
    if(is_bang(t) || tok_eq(t, "-not")) {
        next();
        node_t *o = parse_unary();
        if(o == NULL) die("expected an expression after '!'");
        node_t *n = node_new(K_NOT);
        n->a = o;
        return n;
    }
    if(is_open_paren(t)) {
        next();
        node_t *e = parse_expr();
        if(e == NULL) e = node_new(K_TRUE);
        if(!is_close_paren(peek())) die("expected ')'");
        next();
        return e;
    }
    return parse_primary();
}

static node_t *parse_time(int field, int unit, const char *pred) {
    const char *a = need_arg(pred);
    long long v; int sg;
    if(parse_signed(a, &v, &sg) != 0) die_arg(pred, "invalid time argument");
    node_t *n = node_new(K_TIME);
    n->op = field; n->op2 = unit; n->num = v; n->sign = sg;
    return n;
}

static node_t *parse_primary(void) {
    const char *t = next();
    if(t == NULL) return NULL;
    node_t *n;

    /* ---- options that may appear inside the expression ---- */
    if(tok_eq(t, "-maxdepth") || tok_eq(t, "-mindepth")) {
        const char *a = need_arg(t);
        if(!is_all_digits(a)) die_arg(t, "expected a non-negative integer");
        int v = (int)strtol(a, NULL, 10);
        if(tok_eq(t, "-maxdepth")) g_maxdepth = v; else g_mindepth = v;
        return node_new(K_TRUE);
    }
    if(tok_eq(t, "-depth"))   { g_depth = 1; return node_new(K_TRUE); }
    if(tok_eq(t, "-xdev") || tok_eq(t, "-mount")) { g_xdev = 1; return node_new(K_TRUE); }
    if(tok_eq(t, "-follow"))  { g_follow = 2; return node_new(K_TRUE); }

    /* ---- tests ---- */
    if(tok_eq(t, "-name"))  { n = node_new(K_NAME);  n->s = xstrdup(need_arg(t)); return n; }
    if(tok_eq(t, "-iname")) { n = node_new(K_INAME); n->s = xstrdup(need_arg(t)); return n; }
    if(tok_eq(t, "-path") || tok_eq(t, "-wholename"))
                            { n = node_new(K_PATH);  n->s = xstrdup(need_arg(t)); return n; }
    if(tok_eq(t, "-ipath")) { n = node_new(K_IPATH); n->s = xstrdup(need_arg(t)); return n; }

    if(tok_eq(t, "-type")) {
        const char *a = need_arg(t);
        if(a[0] == 0 || a[1] != 0 || !type_char_is(a[0])) die_arg(t, "invalid type");
        n = node_new(K_TYPE); n->op = a[0]; return n;
    }
    if(tok_eq(t, "-empty")) return node_new(K_EMPTY);

    if(tok_eq(t, "-size")) {
        const char *a = need_arg(t);
        long long v, u; int sg;
        if(parse_size(a, &v, &sg, &u) != 0) die_arg(t, "invalid size");
        n = node_new(K_SIZE); n->num = v; n->sign = sg; n->num2 = u; return n;
    }
    if(tok_eq(t, "-links")) {
        const char *a = need_arg(t);
        long long v; int sg;
        if(parse_signed(a, &v, &sg) != 0) die_arg(t, "invalid link count");
        n = node_new(K_LINKS); n->num = v; n->sign = sg; return n;
    }

    if(tok_eq(t, "-mtime")) return parse_time(T_MTIME, U_DAY, t);
    if(tok_eq(t, "-atime")) return parse_time(T_ATIME, U_DAY, t);
    if(tok_eq(t, "-ctime")) return parse_time(T_CTIME, U_DAY, t);
    if(tok_eq(t, "-mmin"))  return parse_time(T_MTIME, U_MIN, t);
    if(tok_eq(t, "-amin"))  return parse_time(T_ATIME, U_MIN, t);
    if(tok_eq(t, "-cmin"))  return parse_time(T_CTIME, U_MIN, t);

    if(tok_eq(t, "-newer")) {
        const char *a = need_arg(t);
        struct stat ref;
        if(stat(a, &ref) != 0) {
            fprintf(stderr, "find: '-newer' reference '%s': %s\n", a, strerror(errno));
            exit(1);
        }
        n = node_new(K_NEWER); n->num2 = (long long)ref.st_mtime; return n;
    }
    if(tok_eq(t, "-perm")) {
        const char *a = need_arg(t);
        long m; int op;
        if(parse_perm(a, &m, &op) != 0) die_arg(t, "invalid mode");
        n = node_new(K_PERM); n->num2 = m; n->op = op; return n;
    }
    if(tok_eq(t, "-user")) {
        const char *a = need_arg(t);
        long long id = resolve_uid(a);
        if(id < 0) { fprintf(stderr, "find: '-user' argument '%s' is unknown\n", a); exit(1); }
        n = node_new(K_USER); n->num = id; return n;
    }
    if(tok_eq(t, "-group")) {
        const char *a = need_arg(t);
        long long id = resolve_gid(a);
        if(id < 0) { fprintf(stderr, "find: '-group' argument '%s' is unknown\n", a); exit(1); }
        n = node_new(K_GROUP); n->num = id; return n;
    }
    if(tok_eq(t, "-true"))  return node_new(K_TRUE);
    if(tok_eq(t, "-false")) return node_new(K_FALSE);

    /* ---- actions ---- */
    if(tok_eq(t, "-print"))  { n = node_new(K_PRINT);  n->is_action = 1; return n; }
    if(tok_eq(t, "-print0")) { n = node_new(K_PRINT0); n->is_action = 1; return n; }
    if(tok_eq(t, "-printf")) { n = node_new(K_PRINTF); n->is_action = 1; n->s = xstrdup(need_arg(t)); return n; }
    if(tok_eq(t, "-prune"))  return node_new(K_PRUNE);
    if(tok_eq(t, "-quit"))   return node_new(K_QUIT);
    if(tok_eq(t, "-delete")) { g_depth = 1; n = node_new(K_DELETE); n->is_action = 1; return n; }
    if(tok_eq(t, "-exec"))   return parse_exec(K_EXEC);
    if(tok_eq(t, "-ok"))     return parse_exec(K_OK);

    fprintf(stderr, "find: unknown predicate '%s'\n", t);
    exit(1);
}

static node_t *parse_exec(int kind) {
    node_t *n = node_new(kind);
    n->is_action = 1;
    int cap = 8;
    n->argv = (char **)xmalloc(sizeof(char *) * cap);
    n->argc = 0;
    n->flen = 0;
    const char *t;
    int terminated = 0;
    while((t = next()) != NULL) {
        if(strcmp(t, ";") == 0 || strcmp(t, "\\;") == 0) { n->plus = 0; terminated = 1; break; }
        if(strcmp(t, "+") == 0 && kind == K_EXEC)        { n->plus = 1; terminated = 1; break; }
        if(n->argc == cap) {
            cap *= 2;
            n->argv = (char **)realloc(n->argv, sizeof(char *) * cap);
            if(n->argv == NULL) { fprintf(stderr, "find: out of memory\n"); exit(2); }
        }
        n->argv[n->argc++] = xstrdup(t);
        if(strcmp(t, "{}") != 0) n->flen += (int)strlen(t) + 3;
    }
    if(!terminated) die("missing ';' or '+' terminator for -exec/-ok");
    if(n->argc == 0) die("-exec/-ok requires a command");
    if(n->plus) {
        if(g_nplus >= MAX_PLUS_NODES) die("too many '-exec ... +' expressions");
        g_plus[g_nplus++] = n;
    }
    return n;
}

/* ------------------------------------------------------------------ */
/* -printf support                                                     */
/* ------------------------------------------------------------------ */
static void print_dirname(const char *path) {
    const char *slash = strrchr(path, '/');
    if(slash == NULL) { fputs(".", stdout); return; }
    if(slash == path) { fputc('/', stdout); return; }
    for(const char *p = path; p < slash; p++) fputc(*p, stdout);
}

static void print_owner(uid_t uid) {
    session_info_t si;
    if(session_get_by_uid(uid, &si) == 0 && si.user[0]) fputs(si.user, stdout);
    else printf("%d", (int)uid);
}

static void print_group(gid_t gid) {
    session_group_t gi;
    if(session_get_group_by_gid(gid, &gi) == 0 && gi.group[0]) fputs(gi.group, stdout);
    else printf("%d", (int)gid);
}

static void do_printf(node_t *n, ctx_t *c) {
    const char *f = n->s;
    for(const char *p = f; *p; p++) {
        if(*p == '\\') {
            p++;
            switch(*p) {
            case 'n': fputc('\n', stdout); break;
            case 't': fputc('\t', stdout); break;
            case 'r': fputc('\r', stdout); break;
            case '\\': fputc('\\', stdout); break;
            case '0': {
                int v = 0, cnt = 0;
                while(cnt < 3 && p[1] >= '0' && p[1] <= '7') { p++; v = v*8 + (*p - '0'); cnt++; }
                fputc(v, stdout);
                break;
            }
            case 0: break;
            default: fputc(*p, stdout); break;
            }
            continue;
        }
        if(*p != '%') { fputc(*p, stdout); continue; }
        p++;
        switch(*p) {
        case '%': fputc('%', stdout); break;
        case 'p': fputs(c->path, stdout); break;
        case 'f': fputs(c->base, stdout); break;
        case 'd': printf("%d", c->depth); break;
        case 'h': print_dirname(c->path); break;
        case 's': printf("%lld", (long long)c->st.st_size); break;
        case 'y': fputc(type_char(c->st.st_mode), stdout); break;
        case 'Y': fputc(type_char(c->st.st_mode), stdout); break;
        case 'm': printf("%o", (unsigned)(c->st.st_mode & 07777)); break;
        case 'M': fputs(mode_string(c->st.st_mode), stdout); break;
        case 'u': print_owner(c->st.st_uid); break;
        case 'g': print_group(c->st.st_gid); break;
        case 'n': printf("%lu", (unsigned long)c->st.st_nlink); break;
        case 'i': printf("%llu", (unsigned long long)c->st.st_ino); break;
        case 'k': printf("%lld", (long long)((c->st.st_size + 1023) / 1024)); break;
        case 0: break;
        default: fputc(*p, stdout); break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* -exec support                                                       */
/* ------------------------------------------------------------------ */
static char *expand_braces(const char *tok, const char *path) {
    if(strstr(tok, "{}") == NULL) return xstrdup(tok);
    size_t plen = strlen(path), len = 0;
    for(const char *p = tok; *p;) {
        if(p[0] == '{' && p[1] == '}') { len += plen; p += 2; }
        else { len++; p++; }
    }
    char *out = (char *)xmalloc(len + 1), *o = out;
    for(const char *p = tok; *p;) {
        if(p[0] == '{' && p[1] == '}') { memcpy(o, path, plen); o += plen; p += 2; }
        else { *o++ = *p++; }
    }
    *o = 0;
    return out;
}

static char **build_argv1(node_t *n, const char *path) {
    char **av = (char **)xmalloc(sizeof(char *) * (n->argc + 1));
    for(int i = 0; i < n->argc; i++) av[i] = expand_braces(n->argv[i], path);
    av[n->argc] = NULL;
    return av;
}

static void free_argv(char **av) {
    if(av == NULL) return;
    for(int i = 0; av[i]; i++) free(av[i]);
    free(av);
}

/* Launch a command and wait for it. EwokOS waitpid() cannot report the
 * child's exit status, so success is reported once the child has run. */
static int run_command(char **av) {
    fflush(stdout); fflush(stderr);
    pid_t pid = fork();
    if(pid < 0) {
        fprintf(stderr, "find: fork failed: %s\n", strerror(errno));
        g_exit = 1;
        return 0;
    }
    if(pid == 0) {
        execvp(av[0], av);
        fprintf(stderr, "find: '%s': %s\n", av[0] ? av[0] : "?", strerror(errno));
        _exit(127);
    }
    int st = 0;
    waitpid(pid, &st, 0);
    return 1;
}

static void exec_flush(node_t *n) {
    if(n->nbatch == 0) return;
    int extra = n->argc + n->nbatch + 1;
    char **av = (char **)xmalloc(sizeof(char *) * extra);
    int k = 0;
    for(int i = 0; i < n->argc; i++) {
        if(strcmp(n->argv[i], "{}") == 0) {
            for(int j = 0; j < n->nbatch; j++) av[k++] = xstrdup(n->batch[j]);
        } else {
            av[k++] = expand_braces(n->argv[i], n->batch[0]);
        }
    }
    av[k] = NULL;
    run_command(av);
    free_argv(av);
    for(int j = 0; j < n->nbatch; j++) free(n->batch[j]);
    n->nbatch = 0;
    n->batch_bytes = 0;
}

static void exec_batch_add(node_t *n, const char *path) {
    if(n->nbatch == n->batch_cap) {
        int cap = n->batch_cap ? n->batch_cap * 2 : 8;
        n->batch = (char **)realloc(n->batch, sizeof(char *) * cap);
        if(n->batch == NULL) { fprintf(stderr, "find: out of memory\n"); exit(2); }
        n->batch_cap = cap;
    }
    n->batch[n->nbatch++] = xstrdup(path);
    n->batch_bytes += (int)strlen(path) + 3;
    if(n->flen + n->batch_bytes > EXEC_CMD_BUDGET) exec_flush(n);
}

static void flush_all_plus(void) {
    for(int i = 0; i < g_nplus; i++) exec_flush(g_plus[i]);
}

/* ------------------------------------------------------------------ */
/* evaluator                                                           */
/* ------------------------------------------------------------------ */
static int eval_node(node_t *n, ctx_t *c, state_t *s);

static int is_empty(ctx_t *c) {
    if(S_ISDIR(c->st.st_mode)) {
        DIR *d = opendir(c->path);
        if(d == NULL) return 0;
        struct dirent *e;
        int empty = 1;
        while((e = readdir(d)) != NULL) {
            if(strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
            empty = 0; break;
        }
        closedir(d);
        return empty;
    }
    if(S_ISREG(c->st.st_mode)) return c->st.st_size == 0;
    return 0;
}

static int eval_time(node_t *n, ctx_t *c) {
    time_t now = time(NULL);
    time_t t = (n->op == T_ATIME) ? c->st.st_atime :
               (n->op == T_CTIME) ? c->st.st_ctime : c->st.st_mtime;
    long long secs = (long long)now - (long long)t;
    if(secs < 0) secs = 0;
    long long unit = (n->op2 == U_MIN) ? 60 : 86400;
    return cmp_ok(secs / unit, n->num, n->sign);
}

static int eval_perm(node_t *n, ctx_t *c) {
    mode_t m = c->st.st_mode & 07777;
    mode_t want = (mode_t)n->num2;
    if(want == 0) return 1;                     /* -perm /0 and -perm -0 always match */
    if(n->op == PERM_ANY) return (m & want) != 0;
    if(n->op == PERM_ALL) return (m & want) == want;
    return m == want;
}

static int do_delete(ctx_t *c) {
    int r = S_ISDIR(c->st.st_mode) ? rmdir(c->path) : unlink(c->path);
    if(r != 0) {
        fprintf(stderr, "find: cannot delete '%s': %s\n", c->path, strerror(errno));
        g_exit = 1;
        return 0;
    }
    return 1;
}

static int do_ok(node_t *n, ctx_t *c) {
    char **av = build_argv1(n, c->path);
    fputc('<', stderr);
    for(int i = 0; av[i]; i++) fprintf(stderr, " %s", av[i]);
    fputs(" ? ", stderr);
    fflush(stderr);
    int ch = getchar();
    int yes = (ch == 'y' || ch == 'Y');
    while(ch != '\n' && ch != EOF) ch = getchar();
    if(!yes) { free_argv(av); return 0; }
    int r = run_command(av);
    free_argv(av);
    return r;
}

static int eval_node(node_t *n, ctx_t *c, state_t *s) {
    if(n == NULL) return 1;
    switch(n->kind) {
    case K_AND:  return eval_node(n->a, c, s) ? eval_node(n->b, c, s) : 0;
    case K_OR:   return eval_node(n->a, c, s) ? 1 : eval_node(n->b, c, s);
    case K_NOT:  return !eval_node(n->a, c, s);
    case K_TRUE: return 1;
    case K_FALSE:return 0;

    case K_NAME:  return glob_match(n->s, c->base, 0);
    case K_INAME: return glob_match(n->s, c->base, 1);
    case K_PATH:  return glob_match(n->s, c->path, 0);
    case K_IPATH: return glob_match(n->s, c->path, 1);
    case K_TYPE:  return type_match(n->op, c->st.st_mode);
    case K_EMPTY: return is_empty(c);
    case K_SIZE: {
        long long u = n->num2 ? (c->st.st_size + n->num2 - 1) / n->num2 : 0;
        return cmp_ok(u, n->num, n->sign);
    }
    case K_LINKS: return cmp_ok((long long)c->st.st_nlink, n->num, n->sign);
    case K_TIME:  return eval_time(n, c);
    case K_NEWER: return (long long)c->st.st_mtime > n->num2;
    case K_PERM:  return eval_perm(n, c);
    case K_USER:  return (long long)c->st.st_uid == n->num;
    case K_GROUP: return (long long)c->st.st_gid == n->num;

    /* actions */
    case K_PRINT:  fputs(c->path, stdout); fputc('\n', stdout); return 1;
    case K_PRINT0: fputs(c->path, stdout); fputc(0, stdout); return 1;
    case K_PRINTF: do_printf(n, c); return 1;
    case K_PRUNE:  s->prune = 1; return 1;
    case K_QUIT:   s->quit = 1; return 1;
    case K_DELETE: return do_delete(c);
    case K_EXEC:
        if(n->plus) { exec_batch_add(n, c->path); return 1; }
        { char **av = build_argv1(n, c->path); int r = run_command(av); free_argv(av); return r; }
    case K_OK:   return do_ok(n, c);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* traversal                                                           */
/* ------------------------------------------------------------------ */
static void walk(const char *path, int depth, dev_t root_dev, state_t *s);

static void walk_children(const char *dir, int child_depth, dev_t root_dev, state_t *s) {
    DIR *d = opendir(dir);
    if(d == NULL) {
        fprintf(stderr, "find: '%s': %s\n", dir, strerror(errno));
        g_exit = 1;
        return;
    }
    struct dirent *e;
    char child[FIND_MAX_PATH];
    while((e = readdir(d)) != NULL) {
        if(s->quit) break;
        if(e->d_name[0] == 0) continue;
        if(strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        size_t dl = strlen(dir);
        int w;
        if(dl > 0 && dir[dl-1] == '/') w = snprintf(child, sizeof(child), "%s%s", dir, e->d_name);
        else                           w = snprintf(child, sizeof(child), "%s/%s", dir, e->d_name);
        if(w < 0 || (size_t)w >= sizeof(child)) {
            fprintf(stderr, "find: path too long under '%s'\n", dir);
            g_exit = 1;
            continue;
        }
        walk(child, child_depth, root_dev, s);
    }
    closedir(d);
}

/* GNU find defaults to -P (never follow symlinks -> lstat). -L always
 * follows; -H follows only the command-line starting points (depth 0).
 * On EwokOS lstat() is an alias of stat(), so this is a no-op there, but it
 * keeps the semantics correct and portable. */
static int do_stat(const char *path, struct stat *st, int depth) {
    int follow = (g_follow == 2) || (g_follow == 1 && depth == 0);
    return follow ? stat(path, st) : lstat(path, st);
}

static void walk(const char *path, int depth, dev_t root_dev, state_t *s) {
    if(s->quit) return;
    struct stat st;
    if(do_stat(path, &st, depth) != 0) {
        fprintf(stderr, "find: '%s': %s\n", path, strerror(errno));
        g_exit = 1;
        return;
    }
    ctx_t c;
    c.path = path;
    c.base = base_of(path);
    c.st = st;
    c.depth = depth;

    int isdir = S_ISDIR(st.st_mode);
    int descend = isdir && (g_maxdepth < 0 || depth < g_maxdepth);
    if(descend && g_xdev && st.st_dev != root_dev) descend = 0;

    if(g_depth) {
        /* post-order: contents first, then the directory itself */
        if(descend) walk_children(path, depth + 1, root_dev, s);
        if(!s->quit && depth >= g_mindepth) { s->prune = 0; eval_node(g_root, &c, s); }
    } else {
        if(depth >= g_mindepth) {
            s->prune = 0;
            eval_node(g_root, &c, s);
            if(s->prune) descend = 0;
        }
        if(!s->quit && descend) walk_children(path, depth + 1, root_dev, s);
    }
}

/* ------------------------------------------------------------------ */
/* implicit -print                                                     */
/* ------------------------------------------------------------------ */
static int has_action(node_t *n) {
    if(n == NULL) return 0;
    if(n->is_action) return 1;
    return has_action(n->a) || has_action(n->b);
}

static void apply_implicit_print(void) {
    if(g_root == NULL) {
        g_root = node_new(K_PRINT);
        g_root->is_action = 1;
        return;
    }
    if(!has_action(g_root)) {
        node_t *p = node_new(K_PRINT);
        p->is_action = 1;
        node_t *and = node_new(K_AND);
        and->a = g_root;
        and->b = p;
        g_root = and;
    }
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
static void usage(void) {
    fputs(
"usage: find [-H|-L|-P] [path...] [expression]\n"
"\n"
"tests:   -name P -iname P -path P -ipath P -type [fdlcbps] -empty\n"
"         -size [+|-]N[cwbkMG] -links [+|-]N -perm [/|-]MODE\n"
"         -mtime|-atime|-ctime [+|-]N   -mmin|-amin|-cmin [+|-]N\n"
"         -newer FILE -user U -group G -true -false\n"
"options: -maxdepth N -mindepth N -depth -xdev\n"
"actions: -print -print0 -printf FMT -prune -quit -delete\n"
"         -exec CMD {} \\;   -exec CMD {} +   -ok CMD {} \\;\n"
"ops:     ! -not   -a -and (juxtaposition)   -o -or   ( ... )\n",
    stdout);
}

int main(int argc, char **argv) {
    g_argv = argv;
    g_argc = argc;
    int i = 1;

    /* leading global options (must precede paths) */
    while(i < argc) {
        const char *a = argv[i];
        if(strcmp(a, "-H") == 0) { g_follow = 1; i++; continue; }
        if(strcmp(a, "-L") == 0) { g_follow = 2; i++; continue; }
        if(strcmp(a, "-P") == 0) { g_follow = 0; i++; continue; }
        if(strcmp(a, "--help") == 0 || strcmp(a, "-help") == 0) { usage(); return 0; }
        if(strcmp(a, "--version") == 0 || strcmp(a, "-version") == 0)
            { printf("find (EwokOS) 1.0\n"); return 0; }
        if(strcmp(a, "-D") == 0 || strcmp(a, "-O") == 0) { i += 2; continue; }
        break;
    }

    /* starting-point paths */
    char **starts = (char **)xmalloc(sizeof(char *) * (argc + 1));
    int nstarts = 0;
    while(i < argc) {
        const char *a = argv[i];
        if(a[0] == '-' || is_open_paren(a) || is_close_paren(a) ||
           is_bang(a) || strcmp(a, ",") == 0)
            break;
        starts[nstarts++] = argv[i];
        i++;
    }
    if(nstarts == 0) starts[nstarts++] = ".";

    /* expression */
    g_pi = i;
    g_root = parse_expr();
    apply_implicit_print();

    state_t s;
    s.quit = 0;
    s.prune = 0;
    for(int k = 0; k < nstarts && !s.quit; k++) {
        dev_t rd = 0;
        if(g_xdev) {
            struct stat st;
            if(do_stat(starts[k], &st, 0) == 0) rd = st.st_dev;
        }
        walk(starts[k], 0, rd, &s);
    }
    flush_all_plus();
    fflush(stdout);
    return g_exit;
}
