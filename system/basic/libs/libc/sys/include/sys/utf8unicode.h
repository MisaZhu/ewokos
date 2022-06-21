#ifndef UTF8_UNICODE_H
#define UTF8_UNICODE_H

int utf82unicode_char (unsigned char *ch, int *unicode);
int utf82unicode (unsigned char * utf8_str,
		unsigned short * unicode_str,
		int unicode_str_size);
#endif