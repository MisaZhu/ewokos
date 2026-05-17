#include <string.h>

size_t strspn(const char *s, const char *accept) {
	const char *p = s;

	while(*p != '\0') {
		const char *a = accept;
		int found = 0;

		while(*a != '\0') {
			if(*a == *p) {
				found = 1;
				break;
			}
			++a;
		}

		if(!found)
			break;
		++p;
	}

	return (size_t)(p - s);
}
