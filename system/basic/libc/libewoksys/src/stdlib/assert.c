#include <assert.h>
#include <stdlib.h>

void __assert_func(const char *file, int line, const char *func, const char *expr) {
	(void)file;
	(void)line;
	(void)func;
	(void)expr;
	abort();
}

void __assert_fail(const char *expr, const char *file, int line, const char *func) {
	__assert_func(file, line, func, expr);
}
