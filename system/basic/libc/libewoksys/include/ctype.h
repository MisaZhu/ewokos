#ifndef EWOKOS_LIBC_CTYPE_H
#define EWOKOS_LIBC_CTYPE_H

static inline int isdigit(int c) {
	return c >= '0' && c <= '9';
}

static inline int islower(int c) {
	return c >= 'a' && c <= 'z';
}

static inline int isupper(int c) {
	return c >= 'A' && c <= 'Z';
}

static inline int isalpha(int c) {
	return islower(c) || isupper(c);
}

static inline int isalnum(int c) {
	return isalpha(c) || isdigit(c);
}

static inline int isblank(int c) {
	return c == ' ' || c == '\t';
}

static inline int isspace(int c) {
	return isblank(c) || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

static inline int iscntrl(int c) {
	return (c >= 0 && c < 0x20) || c == 0x7f;
}

static inline int isascii(int c) {
	return (unsigned int)c <= 0x7f;
}

static inline int isprint(int c) {
	return c >= 0x20 && c < 0x7f;
}

static inline int isgraph(int c) {
	return c > 0x20 && c < 0x7f;
}

static inline int isxdigit(int c) {
	return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int ispunct(int c) {
	return isgraph(c) && !isalnum(c);
}

static inline int tolower(int c) {
	return isupper(c) ? (c - 'A' + 'a') : c;
}

static inline int toupper(int c) {
	return islower(c) ? (c - 'a' + 'A') : c;
}

#endif
