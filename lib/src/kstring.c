#include "kstring.h"

/*
 * memcmp copies n bytes from the source buffer to the target buffer. It returns
 * the point32_ter to the target.
 */
extern void *__memcpy32(void *target, const void *source, uint32_t n);
inline void *memcpy(void *target, const void *source, uint32_t n) {
	if(target == NULL || source == NULL)
		return NULL;

	char *target_buffer = (char *) target;
	char *source_buffer = (char *) source;
	uint32_t m = (n / 32);
	if(m > 0) {
		m *= 32;
		__memcpy32(target_buffer, source_buffer, m);
	}

	while(m < n) {
		target_buffer[m] = source_buffer[m];
		++m;
	}
	return target;
}

/* memset fills the given target with given length with the given character. */
void* memset(void *target, int32_t c, uint32_t size) {
	if(target == NULL)
		return NULL;

	char* dst = target;
	char value = (char)c;
	uint32_t align_value;
	align_value = (uint32_t)dst & 0x03;
	if(align_value) {
		align_value = 4 - align_value;
		if(size > align_value) {
			while(align_value) {
				*(uint8_t *)dst = value;
				dst += 1;

				align_value -= 1;
				size -= 1;
			}
		}
		else {
			while(size) {
				*(uint8_t *)dst = value;
				dst += 1;
				size -= 1;
			}
			return target;
		}
	}
	align_value = value | (value << 8);
	align_value = align_value | (align_value << 16);
	while(size >= 4) {
		*(uint32_t *)dst = align_value;
		dst += 4;
		size -= 4;
	}
	while(size) {
		*(uint8_t *)dst = value;
		dst += 1;
		size -= 1;
	}
	return target;
}

/*
inline void *memset(void *target, int32_t c, uint32_t len) {
	char *target_buffer = (char *) target;

	register uint32_t i = 0;
	for (i = 0; i < len; i++)
		target_buffer[i] = (char) c;

	return target;
}
*/

/*
 * strcpy copies the given null-terminated source string int32_to the given target.
 * It returns the point32_ter to the target.
 */
inline char *strcpy(char *target, const char *source) {
	if(source == NULL)
		return target;

	char *result = target;
	while (*source) {
		*target = *source;
		target++;
		source++;
	}

	*target = '\0';
	return result;
}

/*
 * strncpy copies source to the target with size n. At most n - 1 characters
 * will be copied and the resulting target will always be null-terminated. The
 * function returns strlen(source). If return value is >= n, then truncation
 * occured.
 */
inline uint32_t strncpy(char *target, const char *source, uint32_t n) {
	if(source == NULL)
		return 0;

	uint32_t source_len = 0;
	uint32_t i = 0;
	while (i < n && source[i] != '\0') {
		target[i] = source[i];
		i++;
	}
	target[i] = '\0';

	source_len = strlen(source);
	return source_len;
}


/*
 * strcmp compares the given input strings using the lexicographical order, and
 * returns 0 if s1 == s2, returns a negative number if s1 < s2, and returns a 
 * positive number if s1 > s2.
 */
inline int32_t strcmp(const char *s1, const char *s2) {
	if(s1 == NULL || s2 == NULL)
		return -1;
	while (*s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
		s1++;
		s2++;
	}
	return (*s1 - *s2);
}


/*
 * strncmp compares the given input strings upto n characters using the
 * lexicographical order. For return value, please see strcmp.
 */
inline int32_t strncmp(const char *s1, const char *s2, uint32_t n) {
	if(s1 == NULL || s2 == NULL)
		return -1;

	uint32_t i = 0;
	if (n == 0)
		return 0;

	while (*s1 == *s2 && *s1 != '\0' && *s2 != '\0' && i < n - 1) {
		s1++;
		s2++;
		i++;
	}

	return (*s1 - *s2);	
}

/*
 * strchr returns a point32_ter to the first occurence of the given character in the
 * given string. If the character is not found, it returns NULL.
 */
inline char *strchr(const char *str, int32_t character) {
	if(str == NULL)
		return NULL;

	while (*str != '\0' && *str != character)
		str++;
	if (*str == character)
		return (char *) str;
	else
		return NULL;
}

/*
 * strtok tokenizes the given string using the given delimiters. If str != NULL,
 * then it returns a point32_ter to the first token. If str == NULL, then it returns
 * the a point32_ter to the next token of the string used in previous calls. If no
 * more tokens are found, it returns NULL.
 *
 * WARNING: This function changes the original string.
 */
inline char *strtok(char *str, const char *delimiters) {
	if(str == NULL)
		return NULL;

	static char *last = NULL;
	char *token = NULL;
	if (str != NULL)
		last = str;

	token = last;

	/* skip leading delimiters */
	while (*token != '\0' && strchr(delimiters, *token) != NULL)
		token++;

	/* if there were no non-delimiter characters, return NULL */
	if (*token == '\0') {
		last = NULL;
		return NULL;
	}

	/* scan to find where token ends */
	while (*last != '\0' && strchr(delimiters, *last) == NULL)
		last++;

	/* terminate the token, and set where the scan of next token starts */
	if (*last != '\0') {
		*last = '\0';
		last++;
	}

	return token;
}

/* strlen returns the length of the given null-terminated string. */
inline uint32_t strlen(const char *str) {
	if(str == NULL)
		return 0;

	uint32_t length = 0;
	while (*str != '\0') {
		str++;
		length++;
	}
	return length;
}
