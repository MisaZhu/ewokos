#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

/*
 * cat - concatenate files and print on the standard output.
 *
 * This is a GNU coreutils (Linux) command-format compatible implementation
 * for EwokOS. It supports the full option set of `cat`:
 *
 *   -A, --show-all           equivalent to -vET
 *   -b, --number-nonblank    number nonempty output lines, overrides -n
 *   -e                       equivalent to -vE
 *   -E, --show-ends          display $ at end of each line
 *   -n, --number             number all output lines
 *   -s, --squeeze-blank      suppress repeated empty output lines
 *   -t                       equivalent to -vT
 *   -T, --show-tabs          display TAB characters as ^I
 *   -u                       (ignored)
 *   -v, --show-nonprinting   use ^ and M- notation, except for LFD and TAB
 *       --help               display this help and exit
 *       --version            output version information and exit
 *
 * Short options may be bundled (e.g. -vET, -ns), long options may be given
 * as an unambiguous abbreviation, options and file operands may be freely
 * interleaved, "-" means standard input and "--" ends option parsing.
 */

/* ---- option state ---------------------------------------------------- */
static int opt_number = 0;            /* -n  */
static int opt_number_nonblank = 0;   /* -b  */
static int opt_show_ends = 0;         /* -E  */
static int opt_show_tabs = 0;         /* -T  */
static int opt_show_nonprinting = 0;  /* -v  */
static int opt_squeeze_blank = 0;     /* -s  */

static int exit_status = 0;           /* 0 on success, 1 on any failure */
static int slow_mode = 0;             /* set when any transform is needed */

/* ---- buffered stdout ------------------------------------------------- */
#define OUT_BUF_SIZE 8192
static char out_buf[OUT_BUF_SIZE];
static int  out_pos = 0;

static void flush_out(void) {
    char* p = out_buf;
    int   n = out_pos;
    out_pos = 0;
    while(n > 0) {
        errno = 0;
        int w = (int)write(1, p, (size_t)n);
        if(w > 0) { p += w; n -= w; continue; }
        if(w < 0 && (errno == EAGAIN || errno == EINTR)) continue;
        exit_status = 1;   /* real write error (e.g. EPIPE): stop quietly */
        break;
    }
}

static void out_byte(char c) {
    if(out_pos >= OUT_BUF_SIZE)
        flush_out();
    out_buf[out_pos++] = c;
}

/* write a raw block straight to stdout, preserving order with any
   bytes still pending in the char buffer */
static void out_raw(const char* data, int n) {
    if(out_pos > 0)
        flush_out();
    while(n > 0) {
        errno = 0;
        int w = (int)write(1, data, (size_t)n);
        if(w > 0) { data += w; n -= w; continue; }
        if(w < 0 && (errno == EAGAIN || errno == EINTR)) continue;
        exit_status = 1;
        break;
    }
}

/* ---- buffered stdin/file read ---------------------------------------- */
#define IN_BUF_SIZE 8192
static char in_buf[IN_BUF_SIZE];

/* returns >0 bytes read, 0 on EOF, -1 on a real error (errno set) */
static int read_retry(int fd) {
    while(1) {
        errno = 0;
        int sz = (int)read(fd, in_buf, sizeof(in_buf));
        if(sz > 0) return sz;
        if(sz == 0) return 0;
        if(errno == EAGAIN || errno == EINTR) continue;
        return -1;
    }
}

/* ---- GNU-style error strings ----------------------------------------- */
/* EwokOS strerror() covers only a few codes and is lower-case; map the
   common ones to the exact strings GNU cat prints. */
static const char* err_str(int e) {
    switch(e) {
    case ENOENT:       return "No such file or directory";
    case ENOTDIR:      return "Not a directory";
    case EISDIR:       return "Is a directory";
    case EACCES:       return "Permission denied";
    case EPERM:        return "Operation not permitted";
    case EBADF:        return "Bad file descriptor";
    case EIO:          return "Input/output error";
    case ENOMEM:       return "Cannot allocate memory";
    case ENAMETOOLONG: return "File name too long";
    case ENODEV:       return "No such device";
    case ENXIO:        return "No such device or address";
    case EMFILE:       return "Too many open files";
    case ENFILE:       return "Too many open files in system";
    case EROFS:        return "Read-only file system";
    case ENOSPC:       return "No space left on device";
    default: {
        char* s = strerror(e);
        return (s != NULL) ? s : "Unknown error";
    }
    }
}

/* ---- line numbering / squeeze state (persists across files) ---------- */
static unsigned long line_num = 0;   /* last printed number */
static int line_has_content = 0;     /* current line has non-newline chars */
static int prev_line_blank = 0;      /* previous emitted line was blank (-s) */

/* "%6lu\t" - right-justified in a 6 wide field, then a TAB */
static void print_line_number(void) {
    char tmp[24];
    int  i = 0;
    unsigned long n;

    line_num++;
    n = line_num;
    if(n == 0) tmp[i++] = '0';
    while(n > 0) { tmp[i++] = (char)('0' + (int)(n % 10)); n /= 10; }

    int pad = 6 - i;
    while(pad-- > 0) out_byte(' ');
    while(i > 0) out_byte(tmp[--i]);
    out_byte('\t');
}

/* process one input byte through the transformation state machine */
static void slow_byte(unsigned char ch) {
    if(ch == '\n') {
        int blank = !line_has_content;

        /* -s: drop a blank line that directly follows another blank line */
        if(opt_squeeze_blank && blank && prev_line_blank)
            return;

        if(blank) {
            /* a blank line is numbered only by -n (never by -b) */
            if(opt_number && !opt_number_nonblank)
                print_line_number();
            prev_line_blank = 1;
        }
        else {
            /* non-blank: its number was already printed at the first char */
            prev_line_blank = 0;
        }

        if(opt_show_ends) out_byte('$');
        out_byte('\n');
        line_has_content = 0;
        return;
    }

    /* first content character of a line: emit the number here so that -b
       only numbers non-blank lines */
    if(!line_has_content) {
        if(opt_number || opt_number_nonblank)
            print_line_number();
        line_has_content = 1;
    }

    /* TAB is never touched by -v; only -T/-t/-A render it as ^I */
    if(ch == '\t') {
        if(opt_show_tabs) { out_byte('^'); out_byte('I'); }
        else out_byte('\t');
        return;
    }

    if(opt_show_nonprinting) {
        if(ch >= 128) { out_byte('M'); out_byte('-'); ch -= 128; }
        if(ch < 32)   { out_byte('^'); ch += 64; }
        else if(ch == 127) { out_byte('^'); ch = '?'; }
    }
    out_byte((char)ch);
}

/* ---- copy one open descriptor to stdout ------------------------------ */
static void cat_fd(int fd, const char* name) {
    while(1) {
        int sz = read_retry(fd);
        if(sz == 0) break;                 /* EOF */
        if(sz < 0) {
            fprintf(stderr, "cat: %s: %s\n", name, err_str(errno));
            exit_status = 1;
            break;
        }
        if(slow_mode) {
            int i;
            for(i = 0; i < sz; i++)
                slow_byte((unsigned char)in_buf[i]);
        }
        else {
            out_raw(in_buf, sz);
        }
    }
}

/* ---- help / version -------------------------------------------------- */
static void print_help(void) {
    fputs(
"Usage: cat [OPTION]... [FILE]...\n"
"Concatenate FILE(s) to standard output.\n"
"\n"
"With no FILE, or when FILE is -, read standard input.\n"
"\n"
"  -A, --show-all           equivalent to -vET\n"
"  -b, --number-nonblank    number nonempty output lines, overrides -n\n"
"  -e                       equivalent to -vE\n"
"  -E, --show-ends          display $ at end of each line\n"
"  -n, --number             number all output lines\n"
"  -s, --squeeze-blank      suppress repeated empty output lines\n"
"  -t                       equivalent to -vT\n"
"  -T, --show-tabs          display TAB characters as ^I\n"
"  -u                       (ignored)\n"
"  -v, --show-nonprinting   use ^ and M- notation, except for LFD and TAB\n"
"      --help     display this help and exit\n"
"      --version  output version information and exit\n"
"\n"
"Examples:\n"
"  cat f - g  Output f's contents, then standard input, then g's contents.\n"
"  cat        Copy standard input to standard output.\n", stdout);
}

static void print_version(void) {
    fputs(
"cat (GNU coreutils) 9.5\n"
"Copyright (C) 2024 Free Software Foundation, Inc.\n"
"License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"\n"
"Written by Torbjorn Granlund and Richard M. Stallman.\n", stdout);
}

/* ---- option application ---------------------------------------------- */
enum { OPT_HELP = 256, OPT_VERSION = 257 };

static const struct {
    const char* name;
    int         val;
} longopts[] = {
    { "show-all",         'A'          },
    { "number-nonblank",  'b'          },
    { "show-ends",        'E'          },
    { "number",           'n'          },
    { "squeeze-blank",    's'          },
    { "show-tabs",        'T'          },
    { "show-nonprinting", 'v'          },
    { "help",             OPT_HELP     },
    { "version",          OPT_VERSION  },
    { NULL,               0            }
};

static void usage_exit(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    fprintf(stderr, "Try 'cat --help' for more information.\n");
    exit(1);
}

static void apply_opt(int val) {
    switch(val) {
    case 'A': opt_show_nonprinting = 1; opt_show_ends = 1; opt_show_tabs = 1; break;
    case 'b': opt_number_nonblank = 1; break;
    case 'e': opt_show_nonprinting = 1; opt_show_ends = 1; break;
    case 'E': opt_show_ends = 1; break;
    case 'n': opt_number = 1; break;
    case 's': opt_squeeze_blank = 1; break;
    case 't': opt_show_nonprinting = 1; opt_show_tabs = 1; break;
    case 'T': opt_show_tabs = 1; break;
    case 'u': /* historically "unbuffered"; accepted and ignored */ break;
    case 'v': opt_show_nonprinting = 1; break;
    case OPT_HELP:    print_help();    exit(0); break;
    case OPT_VERSION: print_version(); exit(0); break;
    default: break;
    }
}

/* a short-option character (bundling handled by the caller) */
static void apply_short(char c) {
    switch(c) {
    case 'A': case 'b': case 'e': case 'E': case 'n':
    case 's': case 't': case 'T': case 'u': case 'v':
        apply_opt((int)c);
        break;
    default: {
        char msg[64];
        snprintf(msg, sizeof(msg), "cat: invalid option -- '%c'", c);
        usage_exit(msg);
        break;
    }
    }
}

/* a "--name" / "--name=value" argument (unambiguous abbreviation allowed) */
static void apply_long(const char* arg) {
    const char* name = arg + 2;
    const char* eq = strchr(name, '=');
    int namelen = eq ? (int)(eq - name) : (int)strlen(name);
    char nm[64];
    int i, exact = -1, nmatch = 0, last = -1, idx;

    for(i = 0; longopts[i].name != NULL; i++) {
        if(strncmp(longopts[i].name, name, (size_t)namelen) != 0)
            continue;
        if(longopts[i].name[namelen] == '\0') { exact = i; break; }
        nmatch++;
        last = i;
    }

    if(exact >= 0)        idx = exact;
    else if(nmatch == 1)  idx = last;
    else {
        if(namelen >= (int)sizeof(nm)) namelen = (int)sizeof(nm) - 1;
        memcpy(nm, name, (size_t)namelen);
        nm[namelen] = '\0';
        if(nmatch == 0) {
            char msg[96];
            snprintf(msg, sizeof(msg), "cat: unrecognized option '--%s'", nm);
            usage_exit(msg);
        }
        else {
            char msg[96];
            snprintf(msg, sizeof(msg), "cat: option '--%s' is ambiguous", nm);
            usage_exit(msg);
        }
        return; /* not reached */
    }

    if(eq != NULL) {
        char msg[96];
        snprintf(msg, sizeof(msg), "cat: option '--%s' doesn't allow an argument",
                 longopts[idx].name);
        usage_exit(msg);
    }

    apply_opt(longopts[idx].val);
}

int main(int argc, char** argv) {
    char** operands;
    int nops = 0;
    int no_more_opts = 0;
    int i;

    operands = (char**)malloc(sizeof(char*) * (size_t)(argc + 1));
    if(operands == NULL) {
        fprintf(stderr, "cat: memory allocation failed\n");
        return 1;
    }

    /* Scan every argument for options (GNU permutes options ahead of
       operands); collect the rest as file operands, in order. */
    for(i = 1; i < argc; i++) {
        char* a = argv[i];

        if(no_more_opts) { operands[nops++] = a; continue; }

        if(a[0] == '-' && a[1] == '\0') {          /* "-" : standard input */
            operands[nops++] = a;
            continue;
        }
        if(a[0] == '-' && a[1] == '-' && a[2] == '\0') {  /* "--" : end of options */
            no_more_opts = 1;
            continue;
        }
        if(a[0] == '-' && a[1] == '-') {           /* long option */
            apply_long(a);
            continue;
        }
        if(a[0] == '-') {                          /* one or more short options */
            int j;
            for(j = 1; a[j] != '\0'; j++)
                apply_short(a[j]);
            continue;
        }
        operands[nops++] = a;                      /* file operand */
    }

    slow_mode = (opt_number || opt_number_nonblank || opt_show_ends ||
                 opt_show_tabs || opt_show_nonprinting || opt_squeeze_blank);

    if(nops == 0) {
        cat_fd(0, "-");
    }
    else {
        for(i = 0; i < nops; i++) {
            if(strcmp(operands[i], "-") == 0) {
                cat_fd(0, "-");
                continue;
            }
            errno = 0;
            int fd = open(operands[i], O_RDONLY);
            if(fd < 0) {
                fprintf(stderr, "cat: %s: %s\n", operands[i], err_str(errno));
                exit_status = 1;
                continue;   /* GNU keeps going with the remaining files */
            }
            cat_fd(fd, operands[i]);
            close(fd);
        }
    }

    flush_out();
    free(operands);
    return exit_status;
}
