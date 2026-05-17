#include <time.h>

size_t strftime(char *s, size_t max, const char *format, const struct tm *tm) {
	(void)format;
	(void)tm;
	if(max > 0 && s != NULL)
		s[0] = '\0';
	return 0;
}
