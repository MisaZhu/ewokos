#include <stdio.h>
#include <errno.h>

/* EwokOS has no shell pipeline support; popen always fails with ENOSYS so
 * callers can detect the absence and skip command-based features. */
FILE *popen(const char *command, const char *type)
{
	(void)command; (void)type;
	errno = ENOSYS;
	return NULL;
}

int pclose(FILE *stream)
{
	(void)stream;
	errno = ENOSYS;
	return -1;
}
