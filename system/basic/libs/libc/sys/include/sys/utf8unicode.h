#ifndef UTF8_UNICODE_H
#define UTF8_UNICODE_H

#ifdef __cplusplus
extern "C" {
#endif

int utf82unicode_char (unsigned char *ch, int *unicode);
int utf82unicode (unsigned char * utf8_str,
		int str_len,
		unsigned short * unicode_str);

#ifdef __cplusplus
}
#endif
#endif