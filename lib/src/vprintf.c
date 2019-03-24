#include <vprintf.h>
#include <types.h>
#include <stdarg.h>

/* digits */
#define DIGITS "0123456789abcdef"

/* forward declarations for local functions */
static void print_string(outc_func_t outc, void* p, const char *str);
static void print_int(outc_func_t outc, void* p, int number);
static void print_uint_in_base(outc_func_t outc, void* p, uint32_t number, uint32_t base);

/*
 * unsigned_divmod divides numerator and denmoriator, then returns the quotient
 * as result. If remainder_pointer is not NULL, then the function returns the
 * division remainder in remainder_pointer.
 *
 * Algorithm: http://en.wikipedia.org/wiki/Division_algorithm
 */
static uint32_t unsigned_divmod(uint32_t numerator, uint32_t denominator,
			 uint32_t* remainder_pointer) {
	int i = 0;
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

/*
 *   - %s: strings,
 *   - %c: characters,
 *   - %d: signed integers,
 *   - %u: unsigned integers,
 *   - %x: hexadecimal representation of integers.
 */
void v_printf(outc_func_t outc, void* p, const char *format, va_list ap) {
	int format_index = 0;

	while (format[format_index] != 0) {
		char format_flag = 0;

		while (format[format_index] != '%' &&
		       format[format_index] != '\0') {
			outc(format[format_index], p);
			format_index++;
		}

		if (format[format_index] == 0 || format[format_index + 1] == 0)
			break;

		format_flag = format[format_index + 1];
		switch (format_flag) {
		/* string */
		case 's': {
			const char *string_arg = va_arg(ap, char *);
			print_string(outc, p, string_arg);
			break;
		}
		/* char */
		case 'c': {
			outc((char) va_arg(ap, int), p);
			break;
		}
		/* int */
		case 'd': {
			int int_arg = va_arg(ap, int);
			print_int(outc, p, int_arg);
			break;
		}
		/* unsigned int */
		case 'u': {
			uint32_t uint_arg = va_arg(ap, uint32_t);
			print_uint_in_base(outc, p, uint_arg, 10);
			break;
		}
		/* hexadecimal */
		case 'x': {
			int uint_arg = va_arg(ap, uint32_t);
			print_string(outc, p, "0x");
			print_uint_in_base(outc, p, uint_arg, 16);
			break;
		}
		}
		/* skip % and format_flag */
		format_index += 2;
	}
}

static void print_string(outc_func_t outc, void* p, const char *str) {
	while(*str != 0) {
		outc(*str, p);
		str++;
	}
}

static void print_int(outc_func_t outc, void* p, int number) {
	if (number < 0) {
		outc('-', p);
		print_uint_in_base(outc, p, -number, 10);
	} else {
		print_uint_in_base(outc, p, number, 10);
	}
}

static void print_uint_in_base(outc_func_t outc, void* p, uint32_t number, uint32_t base) {
	uint32_t last_digit = 0;
	uint32_t rest = 0;

	rest = unsigned_divmod(number, base, &last_digit);
	if (rest != 0)
		print_uint_in_base(outc, p, rest, base);
	outc(DIGITS[last_digit], p);
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
int snprintf(char *target, int size, const char *format, ...) {
	outc_arg_t arg;
	arg.p = target;
	arg.index = 0;
	arg.size = size;

	va_list ap;

	va_start(ap, format);
	v_printf(outc_sn, &arg, format, ap);
	va_end(ap);

	return arg.index;
}

