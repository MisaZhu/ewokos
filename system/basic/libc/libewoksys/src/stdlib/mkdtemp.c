#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

static void mkdtemp_fill_suffix(char *suffix, unsigned long value) {
    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (int i = 5; i >= 0; --i) {
        suffix[i] = alphabet[value % (sizeof(alphabet) - 1)];
        value /= (sizeof(alphabet) - 1);
    }
}

char *mkdtemp(char *path_template) {
    static unsigned long mkdtemp_seq = 0;
    size_t len;
    unsigned long seed;

    if (path_template == NULL) {
        errno = EINVAL;
        return NULL;
    }

    len = strlen(path_template);
    if (len < 6 || strcmp(path_template + len - 6, "XXXXXX") != 0) {
        errno = EINVAL;
        return NULL;
    }

    seed = ((unsigned long)getpid() << 12) ^ (unsigned long)mkdtemp_seq++;
    for (unsigned long attempt = 0; attempt < 4096; ++attempt) {
        struct stat st;

        mkdtemp_fill_suffix(path_template + len - 6, seed + attempt);
        if (stat(path_template, &st) != 0) {
            if (mkdir(path_template, 0700) == 0)
                return path_template;
            if (errno != EEXIST)
                return NULL;
        }
    }

    errno = EEXIST;
    return NULL;
}
