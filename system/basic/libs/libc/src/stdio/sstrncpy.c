#include <stdio.h>
/*
 * Copy src to dst, truncating or null-padding to always copy n bytes.
 * Return dst.
 */
char *sstrncpy(char *dst, const char *src, size_t n) {
	if (n != 0) {
		char *d = dst;
		const char *s = src;

		do {
			if ((*d++ = *s++) == 0) {
				/* NUL pad the remaining n-1 bytes */
				while (--n != 0)
					*d++ = 0;
				break;
			}
		} while (--n != 0);
	}
	return (dst);
}

