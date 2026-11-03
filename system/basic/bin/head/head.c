/* head - output the first part of file(s).
   Command-line and output-format compatible with GNU coreutils head.

   Prints the first 10 lines (or the count selected by -n/-c) of each FILE to
   standard output.  With more than one FILE, precede each with a header giving
   the file name.  With no FILE, or when FILE is -, read standard input.

   Supported syntax mirrors src/head.c:
     -c, --bytes=[-]NUM    first NUM bytes; leading '-' = all but the last NUM
     -n, --lines=[-]NUM    first NUM lines; leading '-' = all but the last NUM
     -q, --quiet, --silent never print headers
     -v, --verbose         always print headers
     -z, --zero-terminated line delimiter is NUL, not newline
     --help, --version
   NUM may carry a multiplier suffix (b 512, kB 1000, K 1024, MB, M, ... and
   the binary KiB/MiB/... forms).  The obsolete "-NUM[cbl]" first-argument form
   is accepted too.

   The elide-from-end paths keep GNU's observable behaviour but use a strategy
   suited to EwokOS: for tail bytes on a regular file the length comes from
   fstat()/lseek() so nothing is buffered, otherwise a ring buffer bounded by
   min(NUM, input) holds the last NUM units.  stdout is unbuffered here, so the
   raw write() below interleaves in order with the printf() headers. */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>

#define IO_BUFSIZE 8192
#define DEFAULT_NUMBER 10

/* "Essentially unlimited" count, matching GNU head's UINTMAX_MAX sentinel:
   for head it means "output everything", for elide it means "output nothing". */
#define N_INF ((uintmax_t)-1)

/* When to print the "==> FILE <==" filename banners. */
enum header_mode { HEADERS_MULTIPLE = 0, HEADERS_ALWAYS, HEADERS_NEVER };

/* Character that separates lines: '\n', or NUL under -z. */
static char line_end = '\n';

/* Non-zero once the banner policy has selected header printing. */
static int print_headers = 0;

/* Zero after the first banner, so a blank separator precedes every later one. */
static int header_first = 1;

/* Set once standard input has been read, so it can be closed at exit. */
static int have_read_stdin = 0;

static int is_digit(int c) {
    return c >= '0' && c <= '9';
}

static void memory_exhausted(void) {
    fprintf(stderr, "head: memory exhausted\n");
    exit(1);
}

/* Write SIZE bytes from DATA to stdout, looping over short writes and parking
   on EAGAIN/EINTR.  A hard error (e.g. EPIPE when the reader went away) just
   stops the copy, as head is never able to deliver the rest anyway. */
static void out(const void* data, size_t size) {
    const char* p = (const char*)data;
    while(size > 0) {
        ssize_t w = write(1, p, size);
        if(w > 0) {
            p += w;
            size -= (size_t)w;
            continue;
        }
        if(w < 0 && (errno == EAGAIN || errno == EINTR)) {
            errno = 0;
            continue;
        }
        break;
    }
}

/* Read up to SIZE bytes into BUF, retrying EAGAIN/EINTR.  Returns the byte
   count, 0 at EOF, or -1 on a real error (with errno set). */
static ssize_t read_retry(int fd, void* buf, size_t size) {
    while(1) {
        ssize_t r = read(fd, buf, size);
        if(r < 0 && (errno == EAGAIN || errno == EINTR)) {
            errno = 0;
            continue;
        }
        return r;
    }
}

static void error_read(const char* filename, int e) {
    fprintf(stderr, "head: error reading '%s': %s\n", filename, strerror(e));
}

/* GNU write_header(): a blank line before every banner except the first.
   The flush keeps the banner ahead of the raw write() data even where stdout
   is buffered; on EwokOS stdout is unbuffered, so it is a harmless no-op. */
static void write_header(const char* filename) {
    printf("%s==> %s <==\n", header_first ? "" : "\n", filename);
    header_first = 0;
    fflush(stdout);
}

/* Interpret the multiplier suffix at S (b, K/kB/KiB, M, G, ... R, Q).
   Returns 0 and stores the factor in *MULT, -1 for an unknown suffix, or
   -2 when the factor itself overflows uintmax_t. */
static int parse_multiplier(const char* s, uintmax_t* mult) {
    int e;
    uintmax_t base, m;
    int i;

    if(s[0] == '\0') { *mult = 1; return 0; }
    if(s[0] == 'b' && s[1] == '\0') { *mult = 512; return 0; }

    switch(s[0]) {
    case 'k': case 'K': e = 1; break;
    case 'm': case 'M': e = 2; break;
    case 'G': e = 3; break;
    case 'T': e = 4; break;
    case 'P': e = 5; break;
    case 'E': e = 6; break;
    case 'Z': e = 7; break;
    case 'Y': e = 8; break;
    case 'R': e = 9; break;
    case 'Q': e = 10; break;
    default: return -1;
    }

    if(s[1] == '\0')                                       base = 1024; /* K  */
    else if(s[1] == 'B' && s[2] == '\0')                   base = 1000; /* kB */
    else if((s[1] == 'i' || s[1] == 'I') && s[2] == 'B'
            && s[3] == '\0')                               base = 1024; /* KiB */
    else return -1;

    m = 1;
    for(i = 0; i < e; i++) {
        if(m > N_INF / base) return -2;
        m *= base;
    }
    *mult = m;
    return 0;
}

/* Parse a run of decimal digits at S into *OUT, clamping to N_INF on
   overflow.  *END receives the first byte past the digits.  Returns non-zero
   if at least one digit was consumed. */
static int parse_digits(const char* s, uintmax_t* out, const char** end) {
    uintmax_t v = 0;
    int any = 0;
    while(is_digit((unsigned char)*s)) {
        uintmax_t d = (uintmax_t)(*s - '0');
        any = 1;
        if(v > (N_INF - d) / 10) {          /* saturate, keep eating digits */
            v = N_INF;
            while(is_digit((unsigned char)*s)) s++;
            break;
        }
        v = v * 10 + d;
        s++;
    }
    *end = s;
    *out = v;
    return any;
}

static void invalid_number(int count_lines, const char* s) {
    fprintf(stderr, "head: %s: '%s'\n",
            count_lines ? "invalid number of lines" : "invalid number of bytes",
            s);
    exit(1);
}

/* GNU string_to_integer(): decimal digits with an optional multiplier suffix.
   Bad input is a fatal diagnostic; overflow silently saturates to N_INF
   (XTOINT_MAX_QUIET). */
static uintmax_t string_to_integer(int count_lines, const char* s) {
    const char* p = s;
    const char* after;
    uintmax_t val, mult;
    int mr;

    while(*p == ' ' || *p == '\t') p++;
    if(*p == '+') p++;

    if(!is_digit((unsigned char)*p))
        invalid_number(count_lines, s);

    parse_digits(p, &val, &after);

    mr = parse_multiplier(after, &mult);
    if(mr == -1)
        invalid_number(count_lines, s);
    if(mr == -2 || (val != 0 && mult > N_INF / val))
        val = N_INF;
    else
        val *= mult;
    return val;
}

/* Output the first N lines (delimited by line_end) of FD. */
static int head_lines(int fd, const char* filename, uintmax_t n) {
    char buf[IO_BUFSIZE];

    while(n > 0) {
        ssize_t r = read_retry(fd, buf, sizeof(buf));
        ssize_t upto, i;
        uintmax_t seen = 0;
        int found = 0;

        if(r < 0) { error_read(filename, errno); return -1; }
        if(r == 0) break;                       /* EOF before N lines */

        upto = r;
        for(i = 0; i < r; i++) {
            if(buf[i] == line_end && ++seen == n) {
                upto = i + 1;                   /* include the delimiter */
                found = 1;
                break;
            }
        }
        out(buf, (size_t)upto);
        if(found) break;
        n -= seen;
    }
    return 0;
}

/* Output the first N bytes of FD (fewer if EOF comes first). */
static int head_bytes(int fd, const char* filename, uintmax_t n) {
    char buf[IO_BUFSIZE];

    while(n > 0) {
        size_t want = (n > sizeof(buf)) ? sizeof(buf) : (size_t)n;
        ssize_t r = read_retry(fd, buf, want);
        if(r < 0) { error_read(filename, errno); return -1; }
        if(r == 0) break;                       /* EOF: not an error for head */
        out(buf, (size_t)r);
        n -= (uintmax_t)r;
    }
    return 0;
}

/* A ring of whole lines, holding at most N of them. */
typedef struct { char* data; size_t len; } linebuf_t;
typedef struct {
    linebuf_t* ring;
    size_t cap;
    size_t count;
    size_t pos;
    uintmax_t n;
} linering_t;

/* Take ownership of DATA (LEN bytes) as the newest line.  Once the ring is
   full, the oldest line is emitted and its slot reused, so everything except
   the final N lines is written as the input streams past. */
static void linering_push(linering_t* L, char* data, size_t len) {
    if(L->count < L->n) {
        if(L->count == L->cap) {
            size_t ncap = L->cap ? L->cap * 2 : 16;
            linebuf_t* nr;
            if((uintmax_t)ncap > L->n) ncap = (size_t)L->n;
            nr = (linebuf_t*)realloc(L->ring, ncap * sizeof(linebuf_t));
            if(nr == NULL) memory_exhausted();
            L->ring = nr;
            L->cap = ncap;
        }
        L->ring[L->count].data = data;
        L->ring[L->count].len = len;
        L->count++;
    } else {
        out(L->ring[L->pos].data, L->ring[L->pos].len);
        free(L->ring[L->pos].data);
        L->ring[L->pos].data = data;
        L->ring[L->pos].len = len;
        L->pos = (L->pos + 1) % L->cap;         /* cap == n once full */
    }
}

static void linering_free(linering_t* L) {
    size_t i;
    for(i = 0; i < L->count; i++)
        free(L->ring[i].data);
    free(L->ring);
}

/* Output all but the last N lines of FD. */
static int elide_tail_lines(int fd, const char* filename, uintmax_t n) {
    linering_t L;
    char* cur = NULL;
    size_t cur_len = 0, cur_cap = 0;
    char buf[IO_BUFSIZE];
    int rc = 0;

    if(n == 0)                                  /* all but the last 0 lines */
        return head_bytes(fd, filename, N_INF);
    if(n == N_INF)                              /* elide everything */
        return 0;

    L.ring = NULL; L.cap = 0; L.count = 0; L.pos = 0; L.n = n;

    while(1) {
        ssize_t r = read_retry(fd, buf, sizeof(buf));
        ssize_t i;
        if(r < 0) { error_read(filename, errno); rc = -1; break; }
        if(r == 0) break;
        for(i = 0; i < r; i++) {
            char c = buf[i];
            if(cur_len + 1 > cur_cap) {
                size_t ncap = cur_cap ? cur_cap * 2 : 256;
                char* nb = (char*)realloc(cur, ncap);
                if(nb == NULL) memory_exhausted();
                cur = nb;
                cur_cap = ncap;
            }
            cur[cur_len++] = c;
            if(c == line_end) {
                linering_push(&L, cur, cur_len);
                cur = NULL; cur_len = 0; cur_cap = 0;   /* ownership moved */
            }
        }
    }

    /* A trailing partial line (no final delimiter) still counts as a line. */
    if(rc == 0 && cur_len > 0) {
        linering_push(&L, cur, cur_len);
        cur = NULL;
    }

    linering_free(&L);
    free(cur);
    return rc;
}

/* Output all but the last N bytes of FD. */
static int elide_tail_bytes(int fd, const char* filename, uintmax_t n) {
    struct stat st;
    char* ring = NULL;
    size_t cap = 0, count = 0, pos = 0;
    char buf[IO_BUFSIZE];
    int rc = 0;

    if(n == 0)                                  /* all but the last 0 bytes */
        return head_bytes(fd, filename, N_INF);
    if(n == N_INF)                              /* elide everything */
        return 0;

    /* Regular file: the remaining length is known, so emit size-N directly
       without buffering (GNU's lseek optimisation). */
    if(fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
        off_t cur = lseek(fd, 0, SEEK_CUR);
        uintmax_t p = (cur > 0) ? (uintmax_t)cur : 0;
        uintmax_t sz = (uintmax_t)st.st_size;
        uintmax_t avail = (sz > p) ? (sz - p) : 0;
        if(avail <= n) return 0;
        return head_bytes(fd, filename, avail - n);
    }

    /* Pipe/stdin: keep the last N bytes in a ring that grows only up to
       min(N, bytes actually read), evicting the oldest as it fills. */
    while(1) {
        ssize_t r = read_retry(fd, buf, sizeof(buf));
        ssize_t i = 0;
        if(r < 0) { error_read(filename, errno); rc = -1; break; }
        if(r == 0) break;
        while(i < r) {
            size_t avail = (size_t)(r - i);
            if(count < n) {
                size_t room, k;
                if(count == cap) {
                    size_t ncap = cap ? cap * 2 : 256;
                    char* nb;
                    if((uintmax_t)ncap > n) ncap = (size_t)n;
                    nb = (char*)realloc(ring, ncap);
                    if(nb == NULL) memory_exhausted();
                    ring = nb;
                    cap = ncap;
                }
                room = cap - count;
                k = room < avail ? room : avail;
                memcpy(ring + count, buf + i, k);
                count += k;
                i += (ssize_t)k;
            } else {
                size_t room = cap - pos;        /* cap == n here */
                size_t k = room < avail ? room : avail;
                out(ring + pos, k);
                memcpy(ring + pos, buf + i, k);
                i += (ssize_t)k;
                pos += k;
                if(pos == cap) pos = 0;
            }
        }
    }
    free(ring);
    return rc;
}

/* Dispatch one already-open FD according to the parsed mode. */
static int head(int fd, const char* filename, uintmax_t n_units,
        int count_lines, int elide_from_end) {
    if(print_headers)
        write_header(filename);

    if(elide_from_end) {
        if(count_lines)
            return elide_tail_lines(fd, filename, n_units);
        return elide_tail_bytes(fd, filename, n_units);
    }
    if(count_lines)
        return head_lines(fd, filename, n_units);
    return head_bytes(fd, filename, n_units);
}

/* Open FILENAME ("-" for standard input), head it, and close it. */
static int head_file(const char* filename, uintmax_t n_units,
        int count_lines, int elide_from_end) {
    int fd;
    int ok;
    int is_stdin = (strcmp(filename, "-") == 0);

    if(is_stdin) {
        have_read_stdin = 1;
        fd = 0;
        filename = "standard input";
    } else {
        fd = open(filename, O_RDONLY);
        if(fd < 0) {
            fprintf(stderr, "head: cannot open '%s' for reading: %s\n",
                    filename, strerror(errno));
            return -1;
        }
    }

    ok = head(fd, filename, n_units, count_lines, elide_from_end);
    if(!is_stdin)
        close(fd);
    return ok;
}

static void usage(int status) {
    if(status != 0) {
        fputs("Try 'head --help' for more information.\n", stderr);
        exit(status);
    }
    fputs(
"Usage: head [OPTION]... [FILE]...\n"
"Print the first 10 lines of each FILE to standard output.\n"
"With more than one FILE, precede each with a header giving the file name.\n"
"\n"
"With no FILE, or when FILE is -, read standard input.\n"
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"  -c, --bytes=[-]NUM         print the first NUM bytes of each file;\n"
"                             with the leading '-', print all but the last\n"
"                             NUM bytes of each file\n"
"  -n, --lines=[-]NUM         print the first NUM lines instead of the first 10;\n"
"                             with the leading '-', print all but the last\n"
"                             NUM lines of each file\n"
"  -q, --quiet, --silent      never print headers giving file names\n"
"  -v, --verbose              always print headers giving file names\n"
"  -z, --zero-terminated      line delimiter is NUL, not newline\n"
"      --help                 display this help and exit\n"
"      --version              output version information and exit\n"
"\n"
"NUM may have a multiplier suffix:\n"
"b 512, kB 1000, K 1024, MB 1000*1000, M 1024*1024,\n"
"GB 1000*1000*1000, G 1024*1024*1024, and so on for T, P, E, Z, Y.\n"
"Binary prefixes can be used, too: KiB=K, MiB=M, and so on.\n", stdout);
    exit(0);
}

int main(int argc, char** argv) {
    static const struct option long_options[] = {
        {"bytes",           required_argument, NULL, 'c'},
        {"lines",           required_argument, NULL, 'n'},
        {"quiet",           no_argument,       NULL, 'q'},
        {"silent",          no_argument,       NULL, 'q'},
        {"verbose",         no_argument,       NULL, 'v'},
        {"zero-terminated", no_argument,       NULL, 'z'},
        {"help",            no_argument,       NULL, 'H'},
        {"version",         no_argument,       NULL, 'V'},
        {NULL, 0, NULL, 0}
    };

    enum header_mode header_mode = HEADERS_MULTIPLE;
    uintmax_t n_units = DEFAULT_NUMBER;
    int count_lines = 1;
    int elide_from_end = 0;
    int ok = 1;
    int c;

    /* Obsolete "-NUM[cbl]" syntax, honoured only as the first argument.
       Parse it, then hide it from getopt by shifting argv (as GNU does). */
    if(argc > 1 && argv[1][0] == '-' && is_digit((unsigned char)argv[1][1])) {
        char* a = argv[1];
        char* n_string = ++a;
        char* end_n_string;
        char multiplier_char = 0;

        do ++a; while(is_digit((unsigned char)*a));
        end_n_string = a;

        for(; *a; a++) {
            switch(*a) {
            case 'c': count_lines = 0; multiplier_char = 0; break;
            case 'b': case 'k': case 'm':
                count_lines = 0; multiplier_char = *a; break;
            case 'l': count_lines = 1; break;
            case 'q': header_mode = HEADERS_NEVER; break;
            case 'v': header_mode = HEADERS_ALWAYS; break;
            case 'z': line_end = '\0'; break;
            default:
                fprintf(stderr, "head: invalid trailing option -- %c\n", *a);
                usage(1);
            }
        }

        *end_n_string = multiplier_char;
        if(multiplier_char)
            *(++end_n_string) = '\0';

        n_units = string_to_integer(count_lines, n_string);

        argv[1] = argv[0];
        argv++;
        argc--;
    }

    while((c = getopt_long(argc, argv, "c:n:qvz0123456789",
                    long_options, NULL)) != -1) {
        switch(c) {
        case 'c':
            count_lines = 0;
            elide_from_end = (optarg[0] == '-');
            if(elide_from_end) ++optarg;
            n_units = string_to_integer(count_lines, optarg);
            break;
        case 'n':
            count_lines = 1;
            elide_from_end = (optarg[0] == '-');
            if(elide_from_end) ++optarg;
            n_units = string_to_integer(count_lines, optarg);
            break;
        case 'q': header_mode = HEADERS_NEVER; break;
        case 'v': header_mode = HEADERS_ALWAYS; break;
        case 'z': line_end = '\0'; break;
        case 'H': usage(0); break;
        case 'V':
            fputs("head (EwokOS) 1.0\n", stdout);
            return 0;
        default:
            if(is_digit(c))
                fprintf(stderr, "head: invalid trailing option -- %c\n", c);
            usage(1);
        }
    }

    if(header_mode == HEADERS_ALWAYS
            || (header_mode == HEADERS_MULTIPLE && optind < argc - 1))
        print_headers = 1;

    if(optind < argc) {
        int i;
        for(i = optind; i < argc; i++)
            if(head_file(argv[i], n_units, count_lines, elide_from_end) != 0)
                ok = 0;
    } else {
        if(head_file("-", n_units, count_lines, elide_from_end) != 0)
            ok = 0;
    }

    if(have_read_stdin)
        close(0);

    return ok ? 0 : 1;
}
