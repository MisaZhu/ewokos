#include <string.h>

/*
 * memcmp copies n bytes from the source buffer to the target buffer. It returns
 * the pointer to the target.
 */
void *memcpy(void *target, const void *source, size_t n)
{
	char *target_buffer = (char *) target;
	char *source_buffer = (char *) source;
	size_t i = 0;

	for (i = 0; i < n; i++)
		target_buffer[i] = source_buffer[i];

	return target;
}

/*
 * strcpy copies the given null-terminated source string into the given target.
 * It returns the pointer to the target.
 */
char *strcpy(char *target, const char *source)
{
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
size_t strncpy(char *target, const char *source, size_t n)
{
	size_t source_len = 0;
	size_t i = 0;

	while (i + 1 < n && source[i] != '\0') {
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
int strcmp(const char *s1, const char *s2)
{
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
int strncmp(const char *s1, const char *s2, size_t n)
{
	size_t i = 0;

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
 * strchr returns a pointer to the first occurence of the given character in the
 * given string. If the character is not found, it returns NULL.
 */
char *strchr(const char *str, int character)
{
	while (*str != '\0' && *str != character)
		str++;

	if (*str == character)
		return (char *) str;
	else
		return NULL;
}

/*
 * strtok tokenizes the given string using the given delimiters. If str != NULL,
 * then it returns a pointer to the first token. If str == NULL, then it returns
 * the a pointer to the next token of the string used in previous calls. If no
 * more tokens are found, it returns NULL.
 *
 * WARNING: This function changes the original string.
 */
char *strtok(char *str, const char *delimiters)
{
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

/* memset fills the given target with given length with the given character. */
void *memset(void *target, int c, size_t len)
{
	char *target_buffer = (char *) target;

	size_t i = 0;
	for (i = 0; i < len; i++)
		target_buffer[i] = (char) c;

	return target;
}

/* strlen returns the length of the given null-terminated string. */
size_t strlen(const char *str)
{
	size_t length = 0;
	while (*str != '\0') {
		str++;
		length++;
	}

	return length;
}
