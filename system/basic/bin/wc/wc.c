/* wc - print newline, word, and byte counts for files.
   Command-line and output-format compatible with GNU coreutils wc.

   Counting semantics follow GNU wc under the C/POSIX locale (EwokOS has no
   locale or ctype tables): a byte is a character, and a word is a maximal run
   of bytes that are not one of the six C isspace() bytes.  The print width,
   the fixed count order (newline, word, char, byte, max-line-length), the
   single-count-no-padding rule and the "total" line all mirror src/wc.c. */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>

#define IO_BUFSIZE 16384

/* Which counts to print. */
static int print_lines = 0;
static int print_words = 0;
static int print_chars = 0;
static int print_bytes = 0;
static int print_linelength = 0;

/* Print width of each count (see compute_width). */
static int number_width = 7;

/* --total=WHEN */
enum { TOTAL_AUTO = 0, TOTAL_ALWAYS, TOTAL_ONLY, TOTAL_NEVER };
static int total_mode = TOTAL_AUTO;

/* Long-only option sentinels; getopt_long returns val unchanged. */
enum {
    OPT_FILES0 = 256,
    OPT_TOTAL,
    OPT_DEBUG,
    OPT_HELP,
    OPT_VERSION
};

/* Cumulative counts across all inputs processed so far. */
static unsigned long long total_lines = 0;
static unsigned long long total_words = 0;
static unsigned long long total_chars = 0;
static unsigned long long total_bytes = 0;
static long long max_line_length = 0;

/* True once standard input has been read as data. */
static int have_read_stdin = 0;

/* C-locale classification.  Only these six bytes separate words, and only
   printable ASCII contributes a column to the -L display width. */
static int c_isspace(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}
static int c_isprint(unsigned char c) {
    return c >= 0x20 && c <= 0x7e;
}

/* Print one count right-justified in number_width.  GNU wc uses "%*s" for the
   first selected count and " %*s" for every following one; the leading space is
   emitted here for non-first fields. */
static void print_count(const char* digits, int first) {
    int pad = number_width - (int)strlen(digits);
    if(!first)
        putchar(' ');
    while(pad-- > 0)
        putchar(' ');
    fputs(digits, stdout);
}

/* Print the counts for one input (or for the grand total).  FILE is the name to
   append, or NULL to print no name (implicit standard input, --total=only). */
static void write_counts(unsigned long long lines, unsigned long long words,
        unsigned long long chars, unsigned long long bytes,
        long long linelength, const char* file) {
    char buf[32];
    int first = 1;

    if(print_lines) {
        snprintf(buf, sizeof(buf), "%llu", lines);
        print_count(buf, first); first = 0;
    }
    if(print_words) {
        snprintf(buf, sizeof(buf), "%llu", words);
        print_count(buf, first); first = 0;
    }
    if(print_chars) {
        snprintf(buf, sizeof(buf), "%llu", chars);
        print_count(buf, first); first = 0;
    }
    if(print_bytes) {
        snprintf(buf, sizeof(buf), "%llu", bytes);
        print_count(buf, first); first = 0;
    }
    if(print_linelength) {
        snprintf(buf, sizeof(buf), "%lld", linelength);
        print_count(buf, first); first = 0;
    }
    if(file)
        printf(" %s", file);
    putchar('\n');
}

/* Read FD, count it, print its line and fold it into the totals.  FILE_X is the
   name to display (NULL for implicit standard input, "-" for an explicit one).
   Returns 0 on success, non-zero on a read error. */
static int wc_count(int fd, const char* file_x) {
    unsigned long long lines = 0, words = 0, bytes = 0;
    long long linelength = 0, linepos = 0;
    int in_word = 0;
    int err = 0;
    char buf[IO_BUFSIZE];
    int need_scan = print_lines || print_words || print_linelength;

    while(1) {
        ssize_t n = read(fd, buf, IO_BUFSIZE);
        if(n < 0) {
            if(errno == EAGAIN || errno == EINTR) {
                errno = 0;
                continue;
            }
            err = errno;
            break;
        }
        if(n == 0)
            break;

        bytes += (unsigned long long)n;
        if(!need_scan)
            continue;

        for(ssize_t i = 0; i < n; i++) {
            unsigned char c = (unsigned char)buf[i];
            switch(c) {
            case '\n':
                lines++;
                /* fall through */
            case '\r':
            case '\f':
                if(linepos > linelength) linelength = linepos;
                linepos = 0;
                in_word = 0;
                break;
            case '\t':
                linepos += 8 - (linepos % 8);
                in_word = 0;
                break;
            case ' ':
                linepos++;
                /* fall through */
            case '\v':
                in_word = 0;
                break;
            default: {
                int in_word2 = !c_isspace(c);
                linepos += c_isprint(c);
                words += (unsigned long long)((!in_word) & in_word2);
                in_word = in_word2;
                break;
            }
            }
        }
    }
    if(linepos > linelength) linelength = linepos;

    unsigned long long chars = bytes; /* C locale: one byte == one character */

    if(total_mode != TOTAL_ONLY)
        write_counts(lines, words, chars, bytes, linelength, file_x);

    total_lines += lines;
    total_words += words;
    total_chars += chars;
    total_bytes += bytes;
    if(linelength > max_line_length) max_line_length = linelength;

    if(err) {
        fprintf(stderr, "wc: %s: %s\n",
                file_x ? file_x : "standard input", strerror(err));
        return 1;
    }
    return 0;
}

/* Count one operand.  A NULL or "-" operand reads standard input. */
static int wc_file(const char* file) {
    int fd;
    if(file == NULL || strcmp(file, "-") == 0) {
        have_read_stdin = 1;
        fd = 0;
    } else {
        fd = open(file, O_RDONLY);
        if(fd < 0) {
            fprintf(stderr, "wc: %s: %s\n", file, strerror(errno));
            return 1;
        }
    }

    int r = wc_count(fd, file);
    if(fd != 0)
        close(fd);
    return r;
}

/* Read NUL-terminated file names from F ("-" for standard input).  The returned
   array points into one heap buffer that must not be freed before use. */
static char** read_files0(const char* from, int* out_count) {
    int fd;
    if(strcmp(from, "-") == 0)
        fd = 0;
    else {
        fd = open(from, O_RDONLY);
        if(fd < 0) {
            fprintf(stderr, "wc: cannot open %s for reading: %s\n", from, strerror(errno));
            exit(1);
        }
    }

    size_t cap = 4096, len = 0;
    char* data = (char*)malloc(cap);
    if(!data) { fprintf(stderr, "wc: memory exhausted\n"); exit(1); }

    while(1) {
        if(len + IO_BUFSIZE + 1 > cap) {
            size_t ncap = cap * 2;
            while(ncap < len + IO_BUFSIZE + 1) ncap *= 2;
            char* nd = (char*)realloc(data, ncap);
            if(!nd) { fprintf(stderr, "wc: memory exhausted\n"); exit(1); }
            data = nd;
            cap = ncap;
        }
        ssize_t n = read(fd, data + len, IO_BUFSIZE);
        if(n < 0) {
            if(errno == EAGAIN || errno == EINTR) { errno = 0; continue; }
            fprintf(stderr, "wc: %s: %s\n", from, strerror(errno));
            break;
        }
        if(n == 0)
            break;
        len += (size_t)n;
    }
    if(fd != 0)
        close(fd);
    data[len] = '\0';

    int count = 0;
    for(size_t i = 0; i < len; i++)
        if(data[i] == '\0') count++;

    char** files = (char**)malloc((size_t)(count > 0 ? count : 1) * sizeof(char*));
    if(!files) { fprintf(stderr, "wc: memory exhausted\n"); exit(1); }

    int idx = 0;
    for(size_t i = 0, start = 0; i <= len; i++) {
        if(i < len && data[i] != '\0')
            continue;
        files[idx++] = data + start;
        start = i + 1;
    }

    *out_count = count;
    return files;
}

/* Compute the field width for the counts, mirroring GNU compute_number_width:
   the width is the number of digits in the summed sizes of the regular inputs
   (an upper bound on every count), never below 7 when any input is not a
   regular file (e.g. a pipe).  One input with one selected count uses width 1
   so a lone number is printed without padding. */
static int compute_width(char* const* files, int nfiles, int total_flags) {
    if(total_mode == TOTAL_ONLY)
        return 1;
    if(nfiles <= 0)
        return 1;
    if(nfiles == 1 && total_flags == 1)
        return 1;

    int minimum_width = 1;
    unsigned long long regular_total = 0;
    for(int i = 0; i < nfiles; i++) {
        struct stat st;
        int r;
        if(files[i] == NULL || strcmp(files[i], "-") == 0)
            r = fstat(0, &st);
        else
            r = stat(files[i], &st);
        if(r != 0)
            continue;
        if(!S_ISREG(st.st_mode))
            minimum_width = 7;
        else
            regular_total += (unsigned long long)st.st_size;
    }

    int width = 1;
    while(regular_total >= 10) { width++; regular_total /= 10; }
    if(width < minimum_width) width = minimum_width;
    return width;
}

static void usage(int status) {
    if(status != 0) {
        fputs("Try 'wc --help' for more information.\n", stderr);
        exit(status);
    }
    fputs(
"Usage: wc [OPTION]... [FILE]...\n"
"  or:  wc [OPTION]... --files0-from=F\n"
"Print newline, word, and byte counts for each FILE, and a total line if\n"
"more than one FILE is specified.  A word is a nonempty sequence of non white\n"
"space delimited by white space characters or by start or end of input.\n"
"With no FILE, or when FILE is -, read standard input.\n"
"The options below may be used to select which counts are printed, always in\n"
"the following order: newline, word, character, byte, maximum line length.\n"
"  -c, --bytes\n"
"         print the byte counts\n"
"  -m, --chars\n"
"         print the character counts\n"
"  -l, --lines\n"
"         print the newline counts\n"
"      --files0-from=F\n"
"         read input from the files specified by\n"
"         NUL-terminated names in file F;\n"
"         If F is -, read names from standard input\n"
"  -L, --max-line-length\n"
"         print the maximum display width\n"
"  -w, --words\n"
"         print the word counts\n"
"      --total=WHEN\n"
"         when to print a line with total counts;\n"
"         WHEN can be: auto, always, only, never\n"
"      --help     display this help and exit\n"
"      --version  output version information and exit\n", stdout);
    exit(0);
}

int main(int argc, char** argv) {
    char* files_from = NULL;
    int c;

    static struct option longopts[] = {
        {"bytes",           no_argument,       NULL, 'c'},
        {"chars",           no_argument,       NULL, 'm'},
        {"lines",           no_argument,       NULL, 'l'},
        {"words",           no_argument,       NULL, 'w'},
        {"max-line-length", no_argument,       NULL, 'L'},
        {"files0-from",     required_argument, NULL, OPT_FILES0},
        {"total",           required_argument, NULL, OPT_TOTAL},
        {"debug",           no_argument,       NULL, OPT_DEBUG},
        {"help",            no_argument,       NULL, OPT_HELP},
        {"version",         no_argument,       NULL, OPT_VERSION},
        {NULL, 0, NULL, 0}
    };

    while((c = getopt_long(argc, argv, "clLmw", longopts, NULL)) != -1) {
        switch(c) {
        case 'c': print_bytes = 1; break;
        case 'm': print_chars = 1; break;
        case 'l': print_lines = 1; break;
        case 'w': print_words = 1; break;
        case 'L': print_linelength = 1; break;
        case OPT_FILES0: files_from = optarg; break;
        case OPT_TOTAL:
            if(strcmp(optarg, "auto") == 0)         total_mode = TOTAL_AUTO;
            else if(strcmp(optarg, "always") == 0)  total_mode = TOTAL_ALWAYS;
            else if(strcmp(optarg, "only") == 0)    total_mode = TOTAL_ONLY;
            else if(strcmp(optarg, "never") == 0)   total_mode = TOTAL_NEVER;
            else {
                fprintf(stderr, "wc: invalid argument '%s' for '--total'\n", optarg);
                fprintf(stderr, "Valid arguments are: 'auto', 'always', 'only', 'never'\n");
                usage(1);
            }
            break;
        case OPT_DEBUG: break; /* accepted; no line-count acceleration to report */
        case OPT_HELP: usage(0); break;
        case OPT_VERSION:
            fputs("wc (EwokOS) 1.0\n", stdout);
            return 0;
        case '?':
            if(optopt != 0)
                fprintf(stderr, "wc: invalid option -- '%c'\n", (char)optopt);
            usage(1);
            break;
        default:
            usage(1);
        }
    }

    if(!(print_lines || print_words || print_chars || print_bytes || print_linelength))
        print_lines = print_words = print_bytes = 1;

    /* Build the operand list. */
    char** files;
    int nfiles;          /* entries to process (implicit stdin counts as one) */
    int total_operands;  /* operands seen, for the --total=auto decision */

    if(files_from) {
        if(optind < argc) {
            fprintf(stderr, "wc: extra operand '%s'\n", argv[optind]);
            fprintf(stderr, "file operands cannot be combined with --files0-from\n");
            usage(1);
        }
        files = read_files0(files_from, &nfiles);
        total_operands = nfiles;
    } else if(optind < argc) {
        files = argv + optind;
        nfiles = argc - optind;
        total_operands = nfiles;
    } else {
        static char* stdin_only[1] = { NULL };
        files = stdin_only;
        nfiles = 1;
        total_operands = 0;
    }

    int total_flags = print_lines + print_words + print_chars + print_bytes + print_linelength;
    number_width = compute_width(files, nfiles, total_flags);

    int ok = 1;
    for(int i = 0; i < nfiles; i++) {
        const char* f = files[i];
        if(f && f[0] == '\0') {
            fprintf(stderr, "wc: invalid zero-length file name\n");
            ok = 0;
            continue;
        }
        if(wc_file(f) != 0)
            ok = 0;
    }

    if(total_mode != TOTAL_NEVER &&
            (total_mode != TOTAL_AUTO || total_operands > 1)) {
        write_counts(total_lines, total_words, total_chars, total_bytes,
                max_line_length, total_mode != TOTAL_ONLY ? "total" : NULL);
    }

    if(have_read_stdin)
        close(0);

    return ok ? 0 : 1;
}
