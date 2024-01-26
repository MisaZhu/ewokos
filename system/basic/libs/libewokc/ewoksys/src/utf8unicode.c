#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int utf82unicode_char (unsigned char *ch, int *unicode) {
	unsigned char *p = NULL;
	int e = 0, n = 0;
	p = ch;
	if(!p || !unicode)
		return 0;

	if(*p >= 0xfc) {
		/* 6:<11111100> */
		e = (p[0] & 0x01) << 30;
		e |= (p[1] & 0x3f) << 24;
		e |= (p[2] & 0x3f) << 18;
		e |= (p[3] & 0x3f) << 12;
		e |= (p[4] & 0x3f) << 6;
		e |= (p[5] & 0x3f);
		n = 6;
	} else if(*p >= 0xf8) {
		/* 5:<11111000> */
		e = (p[0] & 0x03) << 24;
		e |= (p[1] & 0x3f) << 18;
		e |= (p[2] & 0x3f) << 12;
		e |= (p[3] & 0x3f) << 6;
		e |= (p[4] & 0x3f);
		n = 5;
	} else if(*p >= 0xf0) {
		/* 4:<11110000> */
		e = (p[0] & 0x07) << 18;
		e |= (p[1] & 0x3f) << 12;
		e |= (p[2] & 0x3f) << 6;
		e |= (p[3] & 0x3f);
		n = 4;
	} else if(*p >= 0xe0) {
		/* 3:<11100000> */
		e = (p[0] & 0x0f) << 12;
		e |= (p[1] & 0x3f) << 6;
		e |= (p[2] & 0x3f);
		n = 3;
	} else if(*p >= 0xc0) {
		/* 2:<11000000> */
		e = (p[0] & 0x1f) << 6;
		e |= (p[1] & 0x3f);
		n = 2;
	} else {
		e = p[0];
		n = 1;
	}
	*unicode = e;

	/* Return bytes count of this utf-8 character */
	return n;
}

int utf82unicode (unsigned char * utf8_str,
		int str_len,
		unsigned short * unicode_str)
{
	int unicode = 0;
	int i = 0, n = 0;
	int count = 0;
	unsigned char *s = NULL;
	unsigned short *e = NULL;

	s = utf8_str;
	e = unicode_str;

	if (!utf8_str || !unicode_str)
		return 0;

	while (i<str_len) {
		if ((n = utf82unicode_char (s+i, &unicode)) > 0) {
			++count;
			*e = (unsigned short) unicode;
			e++;
			*e = 0;
			/* Step to next utf-8 character */
			i += n;
		} else {
			/* Converting error occurs */
			return count;
		}
	}
	return count;
}

#ifdef __cplusplus
}
#endif