#include <string.h>

char *strpbrk(const char *s, const char *accept) {
	const char *p = s;

	while(*p != '\0') {
		const char *a = accept;

		while(*a != '\0') {
			if(*a == *p)
				return (char *)p;
			++a;
		}
		++p;
	}

	return NULL;
}
