#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

#define MD5_BLOCK_SIZE 64
#define MD5_DIGEST_SIZE 16
#define READ_BUFFER_SIZE 4096

typedef struct {
    uint32_t state[4];
    uint64_t length;
    uint8_t buffer[MD5_BLOCK_SIZE];
    size_t buffer_length;
} md5_context_t;

typedef struct {
    int binary;
    int check;
    int tag;
    int quiet;
    int status;
    int strict;
    int ignore_missing;
    int warn;
} options_t;

static const uint32_t md5_constants[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const uint8_t md5_shifts[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

static uint32_t rotate_left(uint32_t value, uint32_t count) {
    return (value << count) | (value >> (32 - count));
}

static uint32_t load_le32(const uint8_t* p) {
    return (uint32_t)p[0] |
        ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) |
        ((uint32_t)p[3] << 24);
}

static void store_le32(uint8_t* p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void md5_transform(md5_context_t* context, const uint8_t block[MD5_BLOCK_SIZE]) {
    uint32_t words[16];
    uint32_t a = context->state[0];
    uint32_t b = context->state[1];
    uint32_t c = context->state[2];
    uint32_t d = context->state[3];

    for(int i = 0; i < 16; i++)
        words[i] = load_le32(block + i * 4);

    for(uint32_t i = 0; i < 64; i++) {
        uint32_t f;
        uint32_t g;
        uint32_t saved_d;

        if(i < 16) {
            f = (b & c) | ((~b) & d);
            g = i;
        }
        else if(i < 32) {
            f = (d & b) | ((~d) & c);
            g = (5 * i + 1) % 16;
        }
        else if(i < 48) {
            f = b ^ c ^ d;
            g = (3 * i + 5) % 16;
        }
        else {
            f = c ^ (b | (~d));
            g = (7 * i) % 16;
        }

        saved_d = d;
        d = c;
        c = b;
        b += rotate_left(a + f + md5_constants[i] + words[g], md5_shifts[i]);
        a = saved_d;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
}

static void md5_init(md5_context_t* context) {
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
    context->length = 0;
    context->buffer_length = 0;
}

static void md5_update(md5_context_t* context, const uint8_t* data, size_t length) {
    context->length += length;

    if(context->buffer_length != 0) {
        size_t needed = MD5_BLOCK_SIZE - context->buffer_length;
        if(needed > length)
            needed = length;
        memcpy(context->buffer + context->buffer_length, data, needed);
        context->buffer_length += needed;
        data += needed;
        length -= needed;
        if(context->buffer_length == MD5_BLOCK_SIZE) {
            md5_transform(context, context->buffer);
            context->buffer_length = 0;
        }
    }

    while(length >= MD5_BLOCK_SIZE) {
        md5_transform(context, data);
        data += MD5_BLOCK_SIZE;
        length -= MD5_BLOCK_SIZE;
    }

    if(length != 0) {
        memcpy(context->buffer + context->buffer_length, data, length);
        context->buffer_length += length;
    }
}

static void md5_final(md5_context_t* context, uint8_t digest[MD5_DIGEST_SIZE]) {
    uint8_t padding[MD5_BLOCK_SIZE * 2] = { 0x80 };
    uint8_t length_bytes[8];
    uint64_t bit_length = context->length * 8;
    size_t padding_length = context->buffer_length < 56 ?
        56 - context->buffer_length : 120 - context->buffer_length;

    for(int i = 0; i < 8; i++)
        length_bytes[i] = (uint8_t)(bit_length >> (i * 8));

    md5_update(context, padding, padding_length);
    md5_update(context, length_bytes, sizeof(length_bytes));

    for(int i = 0; i < 4; i++)
        store_le32(digest + i * 4, context->state[i]);
}

static int hash_stream(int fd, uint8_t digest[MD5_DIGEST_SIZE]) {
    uint8_t buffer[READ_BUFFER_SIZE];
    md5_context_t context;

    md5_init(&context);
    while(1) {
        errno = 0;
        int count = read(fd, buffer, sizeof(buffer));
        if(count > 0) {
            md5_update(&context, buffer, count);
            continue;
        }
        if(count == 0)
            break;
        if(errno == EAGAIN || errno == EINTR) {
            usleep(1000);
            continue;
        }
        return -1;
    }

    md5_final(&context, digest);
    return 0;
}

static void print_digest(const uint8_t digest[MD5_DIGEST_SIZE]) {
    static const char hex[] = "0123456789abcdef";
    char text[MD5_DIGEST_SIZE * 2 + 1];

    for(int i = 0; i < MD5_DIGEST_SIZE; i++) {
        text[i * 2] = hex[digest[i] >> 4];
        text[i * 2 + 1] = hex[digest[i] & 0x0f];
    }
    text[MD5_DIGEST_SIZE * 2] = 0;
    fputs(text, stdout);
}

static int needs_escaping(const char* filename) {
    return strchr(filename, '\\') != NULL || strchr(filename, '\n') != NULL;
}

static void print_filename(FILE* stream, const char* filename, int escape) {
    if(!escape) {
        fputs(filename, stream);
        return;
    }

    while(*filename != 0) {
        if(*filename == '\\')
            fputs("\\\\", stream);
        else if(*filename == '\n')
            fputs("\\n", stream);
        else
            fputc(*filename, stream);
        filename++;
    }
}

static void print_checksum(const char* filename, const uint8_t digest[MD5_DIGEST_SIZE],
        const options_t* options) {
    if(options->tag) {
        fputs("MD5 (", stdout);
        fputs(filename, stdout);
        fputs(") = ", stdout);
        print_digest(digest);
        fputc('\n', stdout);
        return;
    }

    int escape = needs_escaping(filename);
    if(escape)
        fputc('\\', stdout);
    print_digest(digest);
    fprintf(stdout, " %c", options->binary ? '*' : ' ');
    print_filename(stdout, filename, escape);
    fputc('\n', stdout);
}

static int checksum_file(const char* filename, const options_t* options) {
    uint8_t digest[MD5_DIGEST_SIZE];
    int use_stdin = strcmp(filename, "-") == 0;
    int fd = use_stdin ? 0 : open(filename, O_RDONLY);

    if(fd < 0) {
        fprintf(stderr, "md5sum: %s: %s\n", filename, strerror(errno));
        return 1;
    }

    int result = hash_stream(fd, digest);
    int saved_errno = errno;
    if(!use_stdin)
        close(fd);
    if(result != 0) {
        fprintf(stderr, "md5sum: %s: %s\n", filename, strerror(saved_errno));
        return 1;
    }

    print_checksum(filename, digest, options);
    return 0;
}

static int hex_value(char c) {
    if(c >= '0' && c <= '9')
        return c - '0';
    if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int parse_hex_digest(const char* text, uint8_t digest[MD5_DIGEST_SIZE]) {
    for(int i = 0; i < MD5_DIGEST_SIZE; i++) {
        int high = hex_value(text[i * 2]);
        int low = hex_value(text[i * 2 + 1]);
        if(high < 0 || low < 0)
            return -1;
        digest[i] = (uint8_t)((high << 4) | low);
    }
    return 0;
}

static int unescape_filename(char* filename) {
    char* output = filename;

    while(*filename != 0) {
        if(*filename != '\\') {
            *output++ = *filename++;
            continue;
        }
        filename++;
        if(*filename == '\\')
            *output++ = '\\';
        else if(*filename == 'n')
            *output++ = '\n';
        else
            return -1;
        filename++;
    }
    *output = 0;
    return 0;
}

static int parse_checksum_line(char* line, uint8_t digest[MD5_DIGEST_SIZE], char** filename) {
    if(strncmp(line, "MD5 (", 5) == 0) {
        char* close = NULL;
        char* p = line + 5;
        while((p = strstr(p, ") = ")) != NULL) {
            close = p;
            p += 4;
        }
        if(close == NULL || strlen(close + 4) != MD5_DIGEST_SIZE * 2 ||
                parse_hex_digest(close + 4, digest) != 0)
            return -1;
        *close = 0;
        *filename = line + 5;
        return 0;
    }

    int escaped = line[0] == '\\';
    char* start = line + escaped;
    if(strlen(start) < MD5_DIGEST_SIZE * 2 + 2 ||
            parse_hex_digest(start, digest) != 0 ||
            start[MD5_DIGEST_SIZE * 2] != ' ' ||
            (start[MD5_DIGEST_SIZE * 2 + 1] != ' ' &&
             start[MD5_DIGEST_SIZE * 2 + 1] != '*'))
        return -1;

    *filename = start + MD5_DIGEST_SIZE * 2 + 2;
    if(escaped && unescape_filename(*filename) != 0)
        return -1;
    return 0;
}

static int read_line(int fd, char** line, size_t* capacity) {
    size_t length = 0;

    if(*line == NULL) {
        *capacity = 256;
        *line = malloc(*capacity);
        if(*line == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    while(1) {
        char c;
        errno = 0;
        int count = read(fd, &c, 1);
        if(count < 0 && (errno == EAGAIN || errno == EINTR)) {
            usleep(1000);
            continue;
        }
        if(count < 0)
            return -1;
        if(count == 0) {
            if(length == 0)
                return 0;
            break;
        }

        if(length + 1 >= *capacity) {
            size_t new_capacity = *capacity * 2;
            char* new_line = realloc(*line, new_capacity);
            if(new_line == NULL) {
                errno = ENOMEM;
                return -1;
            }
            *line = new_line;
            *capacity = new_capacity;
        }
        (*line)[length++] = c;
        if(c == '\n')
            break;
    }

    if(length != 0 && (*line)[length - 1] == '\n')
        length--;
    (*line)[length] = 0;
    return 1;
}

static void print_check_result(const char* filename, int matched, const options_t* options) {
    if(options->status || (matched && options->quiet))
        return;

    int escape = needs_escaping(filename);
    if(escape)
        fputc('\\', stdout);
    print_filename(stdout, filename, escape);
    fputs(matched ? ": OK\n" : ": FAILED\n", stdout);
}

static int verify_checksum(const char* filename, const uint8_t expected[MD5_DIGEST_SIZE],
        const options_t* options, int* verified) {
    uint8_t actual[MD5_DIGEST_SIZE];
    int use_stdin = strcmp(filename, "-") == 0;
    int fd = use_stdin ? 0 : open(filename, O_RDONLY);

    if(fd < 0) {
        if(options->ignore_missing && errno == ENOENT)
            return 0;
        if(!options->status)
            fprintf(stderr, "md5sum: %s: %s\n", filename, strerror(errno));
        return 1;
    }

    int result = hash_stream(fd, actual);
    int saved_errno = errno;
    if(!use_stdin)
        close(fd);
    if(result != 0) {
        if(!options->status)
            fprintf(stderr, "md5sum: %s: %s\n", filename, strerror(saved_errno));
        return 1;
    }

    (*verified)++;
    int matched = memcmp(actual, expected, MD5_DIGEST_SIZE) == 0;
    print_check_result(filename, matched, options);
    return matched ? 0 : 1;
}

static int check_file(const char* list_filename, const options_t* options) {
    int use_stdin = strcmp(list_filename, "-") == 0;
    int fd = use_stdin ? 0 : open(list_filename, O_RDONLY);
    if(fd < 0) {
        if(!options->status)
            fprintf(stderr, "md5sum: %s: %s\n", list_filename, strerror(errno));
        return 1;
    }

    char* line = NULL;
    size_t capacity = 0;
    uint32_t line_number = 0;
    int formatted = 0;
    int improper = 0;
    int verified = 0;
    int failed = 0;
    int read_result;

    while((read_result = read_line(fd, &line, &capacity)) > 0) {
        uint8_t expected[MD5_DIGEST_SIZE];
        char* filename;
        line_number++;

        if(parse_checksum_line(line, expected, &filename) != 0) {
            improper++;
            if(options->warn && !options->status)
                fprintf(stderr, "md5sum: %s: %u: improperly formatted MD5 checksum line\n",
                    list_filename, line_number);
            continue;
        }
        formatted++;
        if(verify_checksum(filename, expected, options, &verified) != 0)
            failed = 1;
    }

    int saved_errno = errno;
    free(line);
    if(!use_stdin)
        close(fd);

    if(read_result < 0) {
        if(!options->status)
            fprintf(stderr, "md5sum: %s: %s\n", list_filename, strerror(saved_errno));
        failed = 1;
    }
    if(!formatted) {
        if(!options->status)
            fprintf(stderr, "md5sum: %s: no properly formatted MD5 checksum lines found\n",
                list_filename);
        failed = 1;
    }
    else if(options->ignore_missing && verified == 0) {
        if(!options->status)
            fprintf(stderr, "md5sum: %s: no file was verified\n", list_filename);
        failed = 1;
    }
    if(improper != 0 && !options->status) {
        fprintf(stderr, "md5sum: WARNING: %d %s improperly formatted\n", improper,
            improper == 1 ? "line is" : "lines are");
    }
    if(options->strict && improper != 0)
        failed = 1;

    return failed;
}

static void usage(FILE* stream) {
    fputs("Usage: md5sum [OPTION]... [FILE]...\n"
        "Print or check MD5 (128-bit) checksums.\n"
        "With no FILE, or when FILE is -, read standard input.\n\n"
        "  -b, --binary         read in binary mode\n"
        "  -c, --check          read MD5 sums from FILEs and check them\n"
        "      --tag            create a BSD-style checksum\n"
        "  -t, --text           read in text mode (default)\n"
        "      --quiet          do not print OK for verified files\n"
        "      --status         do not output anything in check mode\n"
        "      --strict         fail on improperly formatted checksum lines\n"
        "      --ignore-missing do not fail for missing files in check mode\n"
        "  -w, --warn           warn about improperly formatted checksum lines\n"
        "      --help           display this help and exit\n"
        "      --version        output version information and exit\n", stream);
}

/* Return the first operand, a negative value for successful early exit, or
 * the argument count plus one for an option error. */
static int parse_options(int argc, char** argv, options_t* options) {
    int index = 1;

    memset(options, 0, sizeof(*options));
    while(index < argc) {
        const char* argument = argv[index];
        if(strcmp(argument, "--") == 0)
            return index + 1;
        if(argument[0] != '-' || argument[1] == 0)
            break;

        if(argument[1] == '-') {
            if(strcmp(argument, "--binary") == 0)
                options->binary = 1;
            else if(strcmp(argument, "--text") == 0)
                options->binary = 0;
            else if(strcmp(argument, "--check") == 0)
                options->check = 1;
            else if(strcmp(argument, "--tag") == 0)
                options->tag = 1;
            else if(strcmp(argument, "--quiet") == 0)
                options->quiet = 1;
            else if(strcmp(argument, "--status") == 0)
                options->status = 1;
            else if(strcmp(argument, "--strict") == 0)
                options->strict = 1;
            else if(strcmp(argument, "--ignore-missing") == 0)
                options->ignore_missing = 1;
            else if(strcmp(argument, "--warn") == 0)
                options->warn = 1;
            else if(strcmp(argument, "--help") == 0) {
                usage(stdout);
                return -1;
            }
            else if(strcmp(argument, "--version") == 0) {
                fputs("md5sum (EwokOS) 1.0\n", stdout);
                return -1;
            }
            else {
                fprintf(stderr, "md5sum: unrecognized option '%s'\n", argument);
                usage(stderr);
                return argc + 1;
            }
        }
        else {
            for(int i = 1; argument[i] != 0; i++) {
                switch(argument[i]) {
                case 'b':
                    options->binary = 1;
                    break;
                case 't':
                    options->binary = 0;
                    break;
                case 'c':
                    options->check = 1;
                    break;
                case 'w':
                    options->warn = 1;
                    break;
                default:
                    fprintf(stderr, "md5sum: invalid option -- '%c'\n", argument[i]);
                    usage(stderr);
                    return argc + 1;
                }
            }
        }
        index++;
    }
    return index;
}

int main(int argc, char** argv) {
    options_t options;
    int first = parse_options(argc, argv, &options);
    if(first < 0)
        return 0;
    if(first > argc)
        return 1;
    if(options.check && options.tag) {
        fputs("md5sum: the --tag option is meaningless when verifying checksums\n", stderr);
        return 1;
    }

    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    int result = 0;
    if(first == argc) {
        const char* filename = "-";
        result = options.check ? check_file(filename, &options) :
            checksum_file(filename, &options);
    }
    else {
        for(int i = first; i < argc; i++) {
            int current = options.check ? check_file(argv[i], &options) :
                checksum_file(argv[i], &options);
            if(current != 0)
                result = 1;
        }
    }
    return result;
}
