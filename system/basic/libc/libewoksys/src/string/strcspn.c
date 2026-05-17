#include <string.h>

size_t strcspn(const char *s, const char *reject) {
	const char *p = s;

	while(*p != '\0') {
		const char *r = reject;

		while(*r != '\0') {
			if(*r == *p)
				return (size_t)(p - s);
			++r;
		}
		++p;
	}

	return (size_t)(p - s);
}
