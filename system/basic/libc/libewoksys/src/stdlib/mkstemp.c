#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

static void mkstemp_fill_suffix(char *suffix, unsigned long value) {
	static const char alphabet[] =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	for (int i = 5; i >= 0; --i) {
		suffix[i] = alphabet[value % (sizeof(alphabet) - 1)];
		value /= (sizeof(alphabet) - 1);
	}
}

int mkstemp(char *path_template) {
	static unsigned long mkstemp_seq = 0;
	size_t len;
	unsigned long seed;

	if (path_template == NULL) {
		errno = EINVAL;
		return -1;
	}

	len = strlen(path_template);
	if (len < 6 || strcmp(path_template + len - 6, "XXXXXX") != 0) {
		errno = EINVAL;
		return -1;
	}

	seed = ((unsigned long)getpid() << 12) ^ (unsigned long)mkstemp_seq++;
	for (unsigned long attempt = 0; attempt < 4096; ++attempt) {
		mkstemp_fill_suffix(path_template + len - 6, seed + attempt);
		int fd = open(path_template, O_CREAT | O_EXCL | O_RDWR, 0600);
		if (fd >= 0) {
			return fd;
		}
		if (errno != EEXIST) {
			return -1;
		}
	}

	errno = EEXIST;
	return -1;
}
