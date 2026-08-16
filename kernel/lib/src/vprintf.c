#include <vprintf.h>
#include <kstring.h>
#include <stdarg.h>
#include <stddef.h>

/* digits */
#define DIGITS_CAP "0123456789ABCDEF"
#define DIGITS     "0123456789abcdef"

typedef enum {
    LEN_DEF = 0,
    LEN_L,
    LEN_LL,
    LEN_Z,
    LEN_T,
    LEN_J,
} fmt_len_t;

/* forward declarations for local functions */
static void print_string(outc_func_t outc, void* p, const char *str, int32_t width);
static void print_int(outc_func_t outc, void* p, int64_t number, int32_t width, uint8_t zero);
static void print_uint_in_base(outc_func_t outc, void* p, uint64_t number, uint32_t base, int32_t width, uint8_t zero, uint8_t cap);
static fmt_len_t v_length(const char* format, int32_t* format_index);
static int64_t v_arg_signed(va_list* ap, fmt_len_t len);
static uint64_t v_arg_unsigned(va_list* ap, fmt_len_t len);

static int32_t is_digit(char c) {
    return (c >= '0') && (c <= '9');
}

static inline int32_t s2i(const char *str) {
    int32_t result = 0;
    int32_t neg_multiplier = 1;

    // Check for negative
    if (*str && *str == '-') {
        neg_multiplier = -1;
        str++;
    }

    // Do number
    for (; *str && is_digit(*str); str++) {
        result = (result * 10) + (*str - '0');
    }

    return result * neg_multiplier;
}

static int32_t v_width(const char* format, int32_t format_index, int32_t* w) {
    char s[8];
    int32_t i = 0;

    if(format[format_index] == '-') {
        s[0] = '-';
        i++;
    }

    for(; i< 8; i++) {
        char c = format[format_index+i];
        if(c < '0' || c > '9')
            break;
        s[i] = c;
    }
    s[i] = 0;
    *w = s2i(s);
    return format_index+i;
}

static fmt_len_t v_length(const char* format, int32_t* format_index) {
    if(format[*format_index] == 'l') {
        (*format_index)++;
        if(format[*format_index] == 'l') {
            (*format_index)++;
            return LEN_LL;
        }
        return LEN_L;
    }
    if(format[*format_index] == 'z') {
        (*format_index)++;
        return LEN_Z;
    }
    if(format[*format_index] == 't') {
        (*format_index)++;
        return LEN_T;
    }
    if(format[*format_index] == 'j') {
        (*format_index)++;
        return LEN_J;
    }
    return LEN_DEF;
}

static int64_t v_arg_signed(va_list* ap, fmt_len_t len) {
    switch(len) {
    case LEN_L:
        return (int64_t)va_arg(*ap, long);
    case LEN_LL:
    case LEN_J:
        return (int64_t)va_arg(*ap, long long);
    case LEN_Z:
        return (int64_t)va_arg(*ap, long);
    case LEN_T:
        return (int64_t)va_arg(*ap, ptrdiff_t);
    default:
        return (int64_t)va_arg(*ap, int);
    }
}

static uint64_t v_arg_unsigned(va_list* ap, fmt_len_t len) {
    switch(len) {
    case LEN_L:
        return (uint64_t)va_arg(*ap, unsigned long);
    case LEN_LL:
    case LEN_J:
        return (uint64_t)va_arg(*ap, unsigned long long);
    case LEN_Z:
        return (uint64_t)va_arg(*ap, size_t);
    case LEN_T:
        return (uint64_t)va_arg(*ap, ptrdiff_t);
    default:
        return (uint64_t)va_arg(*ap, unsigned int);
    }
}

/*
 *   - %s: strings,
 *   - %c: characters,
 *   - %d: signed integers,
 *   - %u: unsigned integers,
 *   - %x: hexadecimal representation of integers.
 */
void v_printf(outc_func_t outc, void* p, const char *format, va_list ap) {
    int32_t format_index = 0;

    while (format[format_index] != 0) {
        char format_flag = 0;

        while (format[format_index] != '%' &&
               format[format_index] != '\0') {
            outc(format[format_index], p);
            format_index++;
        }

        if (format[format_index] == 0 || format[format_index + 1] == 0)
            break;

        format_index++;
        int32_t width = 0;
        uint8_t zero = 0;
        if(format[format_index] == '0') {
            format_index++;
            zero = 1;
        }
        format_index = v_width(format, format_index, &width);
        fmt_len_t len = v_length(format, &format_index);
        format_flag = format[format_index];
        switch (format_flag) {
        /* string */
        case 's': {
            const char *string_arg = va_arg(ap, char *);
            print_string(outc, p, string_arg, width);
            break;
        }
        /* char */
        case 'c': {
            outc((char) va_arg(ap, int), p);
            break;
        }
        /* signed integer */
        case 'd': {
            int64_t int_arg = v_arg_signed(&ap, len);
            print_int(outc, p, int_arg, width, zero);
            break;
        }
        case 'i': {
            int64_t int_arg = v_arg_signed(&ap, len);
            print_int(outc, p, int_arg, width, zero);
            break;
        }
        /* unsigned integer */
        case 'u': {
            uint64_t uint_arg = v_arg_unsigned(&ap, len);
            print_uint_in_base(outc, p, uint_arg, 10, width, zero, 0);
            break;
        }
        /* hexadecimal */
        case 'x': {
            uint64_t uint_arg = v_arg_unsigned(&ap, len);
            print_uint_in_base(outc, p, uint_arg, 16, width, zero, 0);
            break;
        }
        case 'X': {
            uint64_t uint_arg = v_arg_unsigned(&ap, len);
            print_uint_in_base(outc, p, uint_arg, 16, width, zero, 1);
            break;
        }
        case 'p': {
            uint64_t ptr_arg = (uint64_t)(uintptr_t)va_arg(ap, void*);
            print_uint_in_base(outc, p, ptr_arg, 16, width > 0 ? width : (int32_t)(sizeof(uintptr_t) * 2), 1, 0);
            break;
        }
        case '%':
            outc('%', p);
            break;
        default:
            outc('%', p);
            if(len == LEN_L) {
                outc('l', p);
            }
            else if(len == LEN_LL) {
                outc('l', p);
                outc('l', p);
            }
            else if(len == LEN_Z) {
                outc('z', p);
            }
            else if(len == LEN_T) {
                outc('t', p);
            }
            else if(len == LEN_J) {
                outc('j', p);
            }
            outc(format_flag, p);
            break;
        }
        /* skip % and format_flag */
        format_index += 1;
    }
}

static void print_string(outc_func_t outc, void* p, const char *str, int32_t width) {
    int32_t len = (int32_t)strlen(str);
    int32_t i = 0;

    if(width < 0) {
        width = -width;
        for(; i<width-len; i++) {
            outc(' ', p);
        }
    }

    while(*str != 0) {
        outc(*str, p);
        str++;
        i++;
        if(width > 0 && i >= width)
            break;
    }

    for(; i< width; i++) {
        outc(' ', p);
    }
}

static void print_int(outc_func_t outc, void* p, int64_t number, int32_t width, uint8_t zero) {
    if (number < 0) {
        outc('-', p);
        width--;
        print_uint_in_base(outc, p, 0ull - (uint64_t)number, 10, width, zero, 0);
    }
    else {
        print_uint_in_base(outc, p, (uint64_t)number, 10, width, zero, 0);
    }
}

static void print_uint_in_base(outc_func_t outc, void* p, uint64_t number, uint32_t base, int32_t width, uint8_t zero, uint8_t cap) {
    char s[65];
    int32_t pos = 0;
    memset(s, 0, sizeof(s));
    do {
        uint32_t digit = (uint32_t)(number % (uint64_t)base);
        s[pos++] = cap ? DIGITS_CAP[digit] : DIGITS[digit];
        number /= (uint64_t)base;
    } while(number != 0 && pos < (int32_t)(sizeof(s) - 1));

    int32_t len = width - pos;
    int32_t i = 0;

    if(zero) {
        for(; i<len; i++) {
            outc('0', p);
        }
    }

    int32_t j = pos - 1;
    while(j >= 0) {
        if(width > 0 && i >= width)
            break;
        outc(s[j--], p);
        i++;
    }

    while(i < width) {
        outc(' ', p);
        i++;
    }
}

typedef struct {
    char* p;
    uint32_t index;
    uint32_t size;
} outc_arg_t;

static void outc_sn(char c, void* p) {
    outc_arg_t* arg = (outc_arg_t*)p;
    if(arg->index >= arg->size)
        return;
    arg->p[arg->index] = c;
    arg->index++;
}

/*
 * sprintf formats the given data and outputs the result into the given character
 * pointer. See vsprintf for the format flags currently supported.
 */
int32_t snprintf(char *target, int32_t size, const char *format, ...) {
    outc_arg_t arg;
    arg.p = target;
    arg.index = 0;
    arg.size = size;

    va_list ap;

    va_start(ap, format);
    v_printf(outc_sn, &arg, format, ap);
    if(arg.size > 0) {
        uint32_t end = arg.index;
        if(end >= arg.size)
            end = arg.size - 1;
        arg.p[end] = 0;
    }
    va_end(ap);

    return arg.index;
}
