#include <vprintf.h>
#include <types.h>
#include <kstring.h>
#include <stdarg.h>

/* digits */
#define DIGITS "0123456789abcdef"

/* forward declarations for local functions */
static void print_string(outc_func_t outc, void* p, const char *str, int32_t width);
static void print_int(outc_func_t outc, void* p, int32_t numberm, int32_t width);
static void print_uint_in_base(outc_func_t outc, void* p, uint32_t number, uint32_t base, int32_t width);

/*
 * unsigned_divmod divides numerator and denmoriator, then returns the quotient
 * as result. If remainder_pointer is not NULL, then the function returns the
 * division remainder in remainder_pointer.
 *
 * Algorithm: http://en.wikipedia.org/wiki/Division_algorithm
 */
static uint32_t unsigned_divmod(uint32_t numerator, uint32_t denominator,
			 uint32_t* remainder_pointer) {
	int32_t i = 0;
	uint32_t quotient = 0;
	uint32_t remainder = 0;

	for (i = 31; i >= 0; i--) {
		uint32_t numerator_bit = ((numerator & (1u << i)) ? 1 : 0);
		
		remainder = (remainder << 1) | numerator_bit;
		if (remainder >= denominator) {
			remainder -= denominator;
			quotient |= (1u << i);
		}
	}

	if (remainder_pointer != NULL)
		(*remainder_pointer) = remainder;

	return quotient;
}

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

/*
 *   - %s: strings,
 *   - %c: characters,
 *   - %d: signed integers,
 *   - %u: unsigned integers,
 *   - %x: hexadecimal representation of integers.
 */
void v_printf(outc_func_t outc, void* p, const char *format, va_list ap) {
	int32_t format_index = 0;
	int32_t width = 0;

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
		format_index = v_width(format, format_index, &width);
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
		/* int32_t */
		case 'd': {
			int32_t int_arg = va_arg(ap, int);
			print_int(outc, p, int_arg, width);
			break;
		}
		/* unsigned int32_t */
		case 'u': {
			uint32_t uint_arg = va_arg(ap, uint32_t);
			print_uint_in_base(outc, p, uint_arg, 10, width);
			break;
		}
		/* hexadecimal */
		case 'x': {
			int32_t uint_arg = va_arg(ap, uint32_t);
			print_uint_in_base(outc, p, uint_arg, 16, width);
			break;
		}
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

static void print_uint_in_base_raw(outc_func_t outc, void* p, uint32_t number, uint32_t base, int32_t* width) {
	uint32_t last_digit = 0;
	uint32_t rest = 0;

	rest = unsigned_divmod(number, base, &last_digit);
	if (rest != 0)
		print_uint_in_base_raw(outc, p, rest, base, width);
	outc(DIGITS[last_digit], p);
	(*width)--;
}

static void print_int(outc_func_t outc, void* p, int32_t number, int32_t width) {
	if (number < 0) {
		outc('-', p);
		width--;
		print_uint_in_base_raw(outc, p, -number, 10, &width);
	}
	else {
		print_uint_in_base_raw(outc, p, number, 10, &width);
	}

	while(width > 0) {
		outc(' ', p);
		width--;
	}
}

static void print_uint_in_base(outc_func_t outc, void* p, uint32_t number, uint32_t base, int32_t width) {
	print_uint_in_base_raw(outc, p, number, base, &width);
	while(width > 0) {
		outc(' ', p);
		width--;
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
	arg.p[arg.index] = 0;
	va_end(ap);

	return arg.index;
}

