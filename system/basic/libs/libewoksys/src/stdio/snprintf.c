#include <string.h>
#include <vprintf.h>

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
	vprintf(outc_sn, &arg, format, ap);
	arg.p[arg.index] = 0;
	va_end(ap);

	return arg.index;
}

